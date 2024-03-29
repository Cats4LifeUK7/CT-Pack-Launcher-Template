﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using ConsoleHaxx.Common;

namespace ConsoleHaxx.Graces
{
	public class CPK
	{
		public const uint CpkMagic = 0x43504B20; // "CPK "
		public const uint TocMagic = 0x544F4320; // "TOC "
		public const uint ETocMagic = 0x45544F43; // "ETOC"
		public const uint ITocMagic = 0x49544F43; // "ITOC"
		
		public UTF Header;

		public UTF ToC;
		public UTF EToC;
		public UTF IToC;

		public uint Version;
		public uint Mode;

		public DirectoryNode Root;

		private DelayedStreamCache Cache;

		public CPK(Stream data)
		{
			EndianReader reader = new EndianReader(data, Endianness.BigEndian);
			EndianReader lereader = new EndianReader(data, Endianness.LittleEndian);

			if (reader.ReadUInt32() != CpkMagic)
				throw new FormatException();

			for (int i = 0; i < 3; i++)
				reader.ReadUInt32(); // Little Endian: 0xFF; section size; 0x00;

			Header = new UTF(data);

			if (Header.Rows.Count != 1)
				throw new FormatException();

			UTF.Row mainrow = Header.Rows[0];

			long tocoffset = (long)(mainrow.FindValue("TocOffset") as UTF.LongValue).Value;
			long etocoffset = (long)(mainrow.FindValue("EtocOffset") as UTF.LongValue).Value;
			long itocoffset = (long)(mainrow.FindValue("ItocOffset") as UTF.LongValue).Value;
			//long dpkitocoffset = (long)(mainrow.FindValue("DpkItoc") as UTF.IntValue).Value;
			long contentoffset = (long)(mainrow.FindValue("ContentOffset") as UTF.LongValue).Value;
			uint filecount = (mainrow.FindValue("Files") as UTF.IntValue).Value;
			UTF.ShortValue version = mainrow.FindValue("Version") as UTF.ShortValue;
			UTF.IntValue mode = mainrow.FindValue("Mode") as UTF.IntValue;
			if (version != null)
				Version = version.Value;
			if (mode != null)
				Mode = mode.Value;

			reader.Position = tocoffset;
			if (reader.ReadUInt32() != TocMagic)
				throw new FormatException();
			for (int i = 0; i < 3; i++)
				reader.ReadUInt32(); // Little Endian: 0xFF; size; 0x00;
			ToC = new UTF(data);
			
			reader.Position = etocoffset;
			if (reader.ReadUInt32() != ETocMagic)
				throw new FormatException();
			for (int i = 0; i < 3; i++)
				reader.ReadUInt32(); // Little Endian: 0xFF; size; 0x00;
			EToC = new UTF(data);

			reader.Position = itocoffset;
			if (reader.ReadUInt32() != ITocMagic)
				throw new FormatException();
			for (int i = 0; i < 3; i++)
				reader.ReadUInt32(); // Little Endian: 0xFF; size; 0x00;
			IToC = new UTF(data);

			Cache = new DelayedStreamCache();

			Root = new DirectoryNode();
			foreach (var filerow in ToC.Rows) {
				UTF.StringValue dirnamevalue = filerow.FindValue("DirName") as UTF.StringValue;
				string dirname = string.Empty;
				if (dirnamevalue != null)
					dirname = dirnamevalue.Value;
				long offset = (long)(filerow.FindValue("FileOffset") as UTF.LongValue).Value;
				string filename = (filerow.FindValue("FileName") as UTF.StringValue).Value;
				uint packedsize = (filerow.FindValue("FileSize") as UTF.IntValue).Value;
				uint filesize = packedsize;

				if (Version == 7) // ToGf
					offset += tocoffset;
				else // Need to check ToG:Wii's version/mode
					offset += contentoffset;

				UTF.IntValue filesizevalue = filerow.FindValue("ExtractSize") as UTF.IntValue;
				if (filesizevalue != null)
					filesize = filesizevalue.Value;
				else { // If we must, read the uncompressed size from the internal compressed file itself
					data.Position = offset;
					EndianReader littlereader = new EndianReader(data, Endianness.LittleEndian);
					if (littlereader.ReadUInt64() == 0)
						filesize = littlereader.ReadUInt32() + 0x100;
				}

				DirectoryNode dir = Root.Navigate(dirname, true) as DirectoryNode;
				Stream substream = new Substream(data, offset, packedsize);
				try {
					if (filesize > packedsize) {
						//throw new Exception();
						Stream compressed = substream;
						substream = new DelayedStream(delegate() {
							Stream ret = new TemporaryStream();
							CpkCompression.Decompress(compressed, ret);
							ret.Position = 0;
							return ret;
						});
						Cache.AddStream(substream);
					}
					dir.AddChild(new FileNode(filename, filesize, substream));
				} catch {
					dir.AddChild(new FileNode(filename, packedsize, substream));
				}
			}

			// TODO: EToc: Groups

			// TODO: IToc: Attributes
		}
	}
}
