﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;
using ConsoleHaxx.Harmonix;

namespace ConsoleHaxx.RawkSD
{
	public class PlatformGH2PS2Disc : Engine
	{
		public static PlatformGH2PS2Disc Instance;
		public static void Initialise()
		{
			Instance = new PlatformGH2PS2Disc();

			PlatformDetection.AddSelection(Instance);
			PlatformDetection.DetectDirectoryNode += new Action<string, DirectoryNode, List<Pair<Engine, Game>>>(PlatformDetection_DetectDirectoryNode);
			PlatformDetection.DetectHarmonixArk += new Action<string, Ark, List<Pair<Engine, Game>>>(PlatformDetection_DetectHarmonixArk);
		}

		static void PlatformDetection_DetectHarmonixArk(string path, Ark ark, List<Pair<Engine, Game>> platforms)
		{
			if (ark.Version <= 2)
				platforms.Add(new Pair<Engine, Game>(Instance, Game.Unknown));
		}

		static void PlatformDetection_DetectDirectoryNode(string path, DirectoryNode root, List<Pair<Engine, Game>> platforms)
		{
			Game game = Platform.DetermineGame(root);

			if (HarmonixMetadata.IsGuitarHero12(game))
				platforms.Add(new Pair<Engine, Game>(Instance, game));
		}

		public override int ID { get { throw new NotImplementedException(); } }

		public override string Name { get { return "Guitar Hero PS2 Disc"; } }

		public override bool AddSong(PlatformData data, SongData song, ProgressIndicator progress)
		{
			DirectoryNode songdir = data.Session["songdir"] as DirectoryNode;

			SongsDTA dta = HarmonixMetadata.GetSongsDTA(song);
			if (dta == null) { // GH1's <addsong />
				dta = HarmonixMetadata.GetSongData(song);
				dta.Song.Cores = new List<int>() { -1, -1, 1, 1 };
				dta.Song.Vols = new List<float>() { 0, 0, 0, 0 };
				dta.Song.Pans = new List<float>() { -1, 1, -1, 1 };
				dta.Song.Tracks.Find(t => t.Name == "guitar").Tracks.AddRange(new int[] { 2, 3 });
				HarmonixMetadata.SetSongsDTA(song, dta.ToDTB());
			}

			dta.Bank = "sfx/tambourine_bank.milo";

			DirectoryNode songnode = songdir.Navigate(song.ID) as DirectoryNode;

			FileNode chartfile = songnode.Find(song.ID + ".mid") as FileNode;
			if (chartfile == null)
				return false;

			for (int coop = 0; coop < 2; coop++) {
				if (coop == 1) {
					song = new SongData(song);
					song.ID += "_coop";
					song.Name += " [coop]";
					if (dta.SongCoop != null) {
						dta.Song = dta.SongCoop;
						dta.SongCoop = null;
					} else
						return false;
				}
				HarmonixMetadata.SetSongsDTA(song, dta.ToDTB());
				FormatData formatdata = new TemporaryFormatData(song, data);

				FileNode songaudiofile = songnode.Find(song.ID + ".vgs") as FileNode;
				if (songaudiofile == null)
					songaudiofile = songnode.Find(song.ID + "_sp.vgs") as FileNode;
				if (songaudiofile == null)
					return false;

				if (data.Game == Game.GuitarHero1)
					ChartFormatGH1.Instance.Create(formatdata, chartfile == null ? null : chartfile.Data);
				else
					ChartFormatGH2.Instance.Create(formatdata, chartfile == null ? null : chartfile.Data, coop == 1);

				AudioFormatVGS.Instance.Create(formatdata, songaudiofile.Data, null);

				data.AddSong(formatdata);
			}

			return true;
		}

		public override PlatformData Create(string path, Game game, ProgressIndicator progress)
		{
			if (File.Exists(path)) {
				if (String.Compare(Path.GetExtension(path), ".ark", true) == 0 || String.Compare(Path.GetExtension(path), ".hdr", true) == 0)
					path = Path.GetDirectoryName(path);
			}

			PlatformData data = new PlatformData(this, game);

			data.Game = Platform.DetermineGame(data);

			Ark ark = data.GetHarmonixArk(path);

			DirectoryNode songdir = ark.Root.Find("songs", true) as DirectoryNode;
			if (songdir == null)
				Exceptions.Error("This is not a Guitar Hero PS2 disc; the songs dir is missing from the ark.");

			FileNode songsdtbfile = ark.Root.Navigate("config/gen/songs.dtb", false, true) as FileNode;
			if (songsdtbfile == null)
				Exceptions.Error("Couldn't find songs.dtb; this is not a Guitar Hero PS2 disc.");

			data.Session["songdir"] = songdir;

			try {
				List<SongsDTA> dtas = new List<SongsDTA>();
				Stream dtbstream = new MemoryStream((int)songsdtbfile.Size);
				CryptedDtbStream.DecryptOld(dtbstream, new EndianReader(songsdtbfile.Data, Endianness.LittleEndian), (int)songsdtbfile.Size);
				dtbstream.Position = 0;
				DTB.NodeTree dtb = DTB.Create(new EndianReader(dtbstream, Endianness.LittleEndian));
				progress.NewTask(dtb.Nodes.Count);
				foreach (DTB.Node node in dtb.Nodes) {
					DTB.NodeTree tree = node as DTB.NodeTree;
					if (tree == null || tree.Nodes[0].Type != 0x00000005 || songdir.Find((tree.Nodes[0] as DTB.NodeString).Text) == null) {
						progress.Progress();
						continue;
					}

					SongsDTA dta = SongsDTA.Create(tree);
					if (dtas.FirstOrDefault(d => d.BaseName == dta.BaseName) != null) {
						progress.Progress();
						continue; // Don't import songs twice
					}
					dtas.Add(dta);

					SongData song = HarmonixMetadata.GetSongData(data, tree);
					try {
						AddSong(data, song, progress);
					} catch (Exception exception) {
						Exceptions.Warning(exception, "Unable to properly parse " + song.Name);
					}
					progress.Progress();
				}
				progress.EndTask();
			} catch (Exception exception) {
				Exceptions.Error(exception, "An error occurred while parsing the Guitar Hero PS2 disc.");
			}

			return data;
		}
	}
}
