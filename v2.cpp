#include <iostream>
#include <windows.h>
#include <MyHeaders/Math.h>
#include <MyHeaders/strmanip.h>
#include <fstream>
#include <conio.h>
#include <vector>
#include <cstdio>



typedef unsigned char byte;
typedef int addr;



int WinWidth;
int WinHeight;

int VPHeight;

int LogY;

int TermY;

addr SelAddr;

std::fstream File;
addr FileSize;
std::string FilePath;

int BytesPerLine;
int AddrChars;

addr StartAddr;

bool WriteMode;

int DataStart;
int ASCIIStart;

std::vector<addr> FindAddrs;




void Init();

void Quit();

void GetSize();

void Redraw();

void Log(std::string msg);
void Term(std::string msg);

void Seek(addr Addr, bool High = 1);
void SeekRel(addr dAddr, bool High = 1);

std::string MvCur(int Line, int x);

std::string GetDataLine(addr Addr);

std::vector<std::string> GetCommand();

void ExeCommand(std::vector<std::string> ArgV);

int GetCH();

std::string CharReplace(byte c);

void DeleteByte();

void Open();

int GetHexit(char c)
{
  if ('0' <= c && c <= '9')
    return c - '0';
  else if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  else if ('A' <= c && c <= 'F')
    return c - 'A' + 10;
  else
    return -1;
}



int main(int argc, char** argv)
{
  if (argc > 2)
  {
    std::cerr << "Usage: HexEditor [<FilePath>]\n\n";
    return 1;
  }


  Init();

  if (argc == 2)
  {
    FilePath = argv[1];
    Open();
    Redraw();
  }

  Log("Note:  This program is not guaranteed to be functional.");
  std::cout << MvCur(TermY, 0);
  byte c = GetCH();
  bool Special = 0;
  while (1)
  {
    if (c == '/')
    {
      Term("/");
      ExeCommand(GetCommand());
      Term("");
    }
    else if (File.is_open())
    {
      switch (c)
      {
        case 0xE0:
          Special = 1;
          break;

        default:
          if (Special)
          {
            switch (c)
            {
              case 0x48:  //^
                SeekRel(-BytesPerLine);
                break;

              case 0x4B:  //<
                SeekRel(-1);
                break;

              case 0x4D:  //>
                if (WriteMode && SelAddr == FileSize - 1)
                {
                  File.seekp(0, std::ios_base::end).put('\0');
                  Redraw();
                }
                SeekRel(1);
                break;

              case 0x50:  //v
                SeekRel(BytesPerLine);
                break;

              case 0x53:  //del
                if (WriteMode)
                  DeleteByte();
                //Log("Finished");
                break;

              default:
                Log("Unhandeled special key: " + UItoS(c, 16, 2));
            }

            Special = 0;
          }
          else if (WriteMode)
          {
            if (GetHexit(c) != -1)
            {
              std::cout << MvCur((SelAddr - StartAddr) / BytesPerLine, AddrChars + 3 * BytesPerLine + ((SelAddr - StartAddr) % BytesPerLine) + 7) << (char)c;
              byte Byte = GetHexit(c) << 4;
              c = GetCH();
              while (GetHexit(c) == -1 && c != 0x1B)
              {
                c = GetCH();
              }

              if (c != 0x1B)
              {
                std::cout << (char)c;
                Byte += GetHexit(c);

                File.seekp(SelAddr).put(Byte);
              }

              SeekRel(0);
            }
          }
          else
          {
            Term(UItoS(c, 16, 2));
          }
      }
    }

    LoopEnd:
    c = GetCH();
  }
}



void Init()
{
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);

  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);

  SelAddr = 0;
  StartAddr = 0;
  WriteMode = 0;

  Redraw();
}

void Quit()
{
  if (File.is_open())
    File.close();
  std::cout << MvCur(0,0) << "\x1B[J";
  exit(0);
}

void GetSize()
{
  std::string Report;

  int Old = 0;
  WinWidth = 1;
  char c;
  while (WinWidth > Old)
  {
    Old = WinWidth;
    std::cout << " \x1B[6n";
    Report = "";
    c = _getch();
    while (c != 'R')
    {
      Report += c;
      c = _getch();
    }
    StoI(Report.substr(Report.find(';') + 1), WinWidth);
  }
  WinWidth = Old;

  Old = 0;
  WinHeight = 1;
  while (WinHeight > Old)
  {
    Old = WinHeight;
    std::cout << "\n\x1B[6n";
    Report = "";
    c = _getch();
    while (c != ';')
    {
      Report += c;
      c = _getch();
    }
    while (c != 'R')
      c = _getch();
    StoI(Report.substr(Report.find('[') + 1), WinHeight);
  }

  std::cout << "\x1B[1;1H\x1B[J";
}

void Redraw()
{
  std::cout << MvCur(0,0) << "\x1B[J";

  GetSize();

  VPHeight = WinHeight - 4;

  LogY = WinHeight - 3;

  TermY = WinHeight - 1;

  //Log("GetSize successful.  Window is " + UItoS(WinHeight) + "x" + UItoS(WinWidth));
  //Sleep(2000);


  if (File.is_open())
  {
    /*Log(std::string("1: File is ") + ((bool)File ? "" : "not ") + "good");
    Sleep(300);//*/

    Open();

    Seek(StartAddr);
  }

  std::cout << MvCur(WinHeight - 4, 0) << "\x1B[m" + std::string(WinWidth, '=');
  std::cout << MvCur(WinHeight - 2, 0) << "\x1B[m" + std::string(WinWidth, '-');
}

void Log(std::string msg)
{
  std::cout << MvCur(LogY, 0) << std::string(WinWidth, ' ');
  std::cout << MvCur(LogY, 0) << /*"\x1B[45m" + */msg;
}
void Term(std::string msg)
{
  std::cout << MvCur(TermY, 0) << std::string(WinWidth, ' ');
  std::cout << MvCur(TermY, 0) << /*"\x1B[100m" + */msg;
}

void Seek(addr Addr, bool High)
{
  if (Addr >= FileSize)
  {
    Log("Attempted to seek out of range");
    Addr = FileSize - 1;
  }
  else if (Addr < 0)
  {
    Log("Attempted to seek below 0");
    Addr = 0;
  }

  StartAddr = (Addr / BytesPerLine) * BytesPerLine;

  int i = 0;
  int Last = MCL::min(VPHeight, (FileSize - 1 - StartAddr) / BytesPerLine + 1);  //max(i) * BytesPerLine cannot be greater than FileSize - 1 => max(i) = (FileSize - 1) / BytesPerLine
  std::cout << MvCur(0,0);
  for ( ; i < Last; i++)
  {
    std::cout << GetDataLine(StartAddr + i * BytesPerLine) << "\n";
  }
  for ( ; i < VPHeight; i++)
  {
    std::cout << std::string(WinWidth, ' ') << "\n";
  }

  if (High)
  {
    SelAddr = Addr;

    /*Log(std::string("3: File is ") + ((bool)File ? "" : "not ") + "good");
    Sleep(300);//*/

    File.seekg(Addr);

    /*Log(std::string("4: File is ") + ((bool)File ? "" : "not ") + "good");
    Sleep(300);//*/

    byte Byte = File.get();
    std::cout << MvCur((SelAddr - StartAddr) / BytesPerLine, DataStart + 3 * ((SelAddr - StartAddr) % BytesPerLine)) << "\x1B[1;4m" + UItoS(Byte, 16, 2) + "\x1B[m";

    std::string ByteRep = CharReplace(Byte);
    std::cout << MvCur((SelAddr - StartAddr) / BytesPerLine, ASCIIStart + ((SelAddr - StartAddr) % BytesPerLine)) << ByteRep.insert(ByteRep.find('m'), ";4");
  }

  std::cout << MvCur(TermY, 0);
}
void SeekRel(addr dAddr, bool High)
{
  if (SelAddr + dAddr < StartAddr)
    Seek(SelAddr + dAddr, 0);
  else if ((SelAddr + dAddr - StartAddr) >= VPHeight * BytesPerLine)
    Seek(SelAddr + dAddr - (VPHeight - 1) * BytesPerLine, 0);
  else
    Seek(StartAddr, 0);

  if (High)
  {
    if (SelAddr + dAddr < 0)
      SelAddr = 0;
    else if (SelAddr + dAddr >= FileSize)
      SelAddr = FileSize - 1;
    else
      SelAddr += dAddr;

    File.seekg(SelAddr);
    byte Byte = File.get();
    //Log("Writing byte 0x" + UItoS(Byte, 16, 2) + " at address 0x" + UItoS(SelAddr, 16, AddrChars) + ".  File is " + ((bool)File ? "" : "not ") + "good.");
    std::cout << MvCur((SelAddr - StartAddr) / BytesPerLine, DataStart + 3 * ((SelAddr - StartAddr) % BytesPerLine)) << "\x1B[1;4m" + UItoS(Byte, 16, 2) + "\x1B[m";

    std::string ByteRep = CharReplace(Byte);
    std::cout << MvCur((SelAddr - StartAddr) / BytesPerLine, ASCIIStart + ((SelAddr - StartAddr) % BytesPerLine)) << ByteRep.insert(ByteRep.find('m'), ";4");
  }

  std::cout << MvCur(TermY, 0);
}

std::string MvCur(int Line, int x)
{
  return "\x1B[" + UItoS(Line + 1) + ";" + UItoS(x + 1) + "H";
}

std::string GetDataLine(addr Addr)
{
  std::string Output = std::string(WinWidth, ' ');
  Output.replace(0, DataStart, "0x" + UItoS(Addr, 16, AddrChars) + " | ");
  Output.replace(ASCIIStart - 2, 2, "| ");
  File.seekg(Addr);
  int ASCIILast = ASCIIStart;
  byte c = '\0';
  for (int i = 0; i < BytesPerLine; i++)
  {
    c = File.get();

    if (!File)
      break;

    Output.replace(DataStart + 3*i, 2, UItoS(c, 16, 2));

    Output.replace(ASCIILast, 1, CharReplace(c));
    ASCIILast += CharReplace(c).length();
  }

  File.clear();

  return Output;
}

std::vector<std::string> GetCommand()
{
  std::vector<std::string> Empty = { };
  byte c = GetCH();
  std::string Comm = "";
  int Pos = 0;
  bool Special = 0;
  while(c != 0x0D)
  {
    switch(c)
    {
      case '\b':
        if (Pos == 0)
          return Empty;
        else
        {
          Comm.erase(Pos - 1, 1);
          --Pos;
          Term("/" + Comm);
          std::cout << MvCur(TermY, Pos + 1);
        }
        break;

      case 0xE0:
        Special = 1;
        break;

      case '\n':
        break;

      case '\r':
        break;

      case '\t':
        break;

      default:
        if (Special)
        {
          switch (c)
          {
            case 0x48:
              Log("Command memory not implemented.");
              std::cout << MvCur(TermY, Pos + 1);
              break;

            case 0x4B:
              --Pos;
              std::cout << "\b";
              break;

            case 0x4D:
              if (Pos < Comm.length())
              {
                std::cout << Comm[Pos];
                ++Pos;
              }
              break;

            case 0x50:
              Pos = Comm.length();
              Term("/" + Comm);
              break;
          }

          Special = 0;
        }
        else
        {
          Comm.insert(Pos++, 1, c);
          Term("/" + Comm);
          std::cout << MvCur(TermY, Pos + 1);
        }
        break;
    }

    c = GetCH();
  }

  //Log("Broke out of loop");
  //Sleep(2000);

  Comm = TrimWhitespace(Comm);

  if (Comm == "")
    return Empty;

  std::vector<std::string> ArgV = { "" };
  bool InQuotes = 0;
  bool BackSlash = 0;
  short int AwaitingNum = 0;
  bool Oct;
  byte Num = 0;
  for (int i = 0; i < Comm.length(); i++)
  {
    if (AwaitingNum)
    {
      int Val = GetHexit(Comm[i]);
      /*if (Val == -1 || (Oct && Val > 7))
      {
        Log(std::string("Expected ") + (Oct ? "octal" : "hexadecimal") + " number: " + Comm.insert(i+1, "\x1B[m").insert(i, "\x1B[;1;4;31m"));
        return Empty;
      }*/
      if (Oct)
      {
        if ((ullint)Val > 7)
        {
          AwaitingNum = 0;
          --i;
        }
        else
          (Num <<= 3) += Val;
      }
      else
      {
        if (Val == -1)
        {
          AwaitingNum = 0;
          --i;
        }
        else
          (Num <<= 4) += Val;
      }

      if (AwaitingNum == 0)
      {
        ArgV.back() += (char)Num;
        Num = 0;
      }
    }
    else if (BackSlash)
    {
      switch (Comm[i])
      {
        case '\'':
        case '\"':
        case '\\':
        case '\?':
          ArgV.back() += Comm[i];
          break;

        case 'a':
          ArgV.back() += '\a';
          break;

        case 'b':
          ArgV.back() += '\b';
          break;

        case 'f':
          ArgV.back() += '\f';
          break;

        case 'n':
          ArgV.back() += '\n';
          break;

        case 'r':
          ArgV.back() += '\r';
          break;

        case 't':
          ArgV.back() += '\t';
          break;

        case 'v':
          ArgV.back() += '\v';
          break;

        case 'x':
          AwaitingNum = 2;
          Oct = 0;
          break;

        default:
          if ('0' <= Comm[i] && Comm[i] <= '7')
          {
            if (Comm[i] >= '4')
            {
              Log(":Invalid octal substitution: " + Comm.insert(i+1, "\x1B[m").insert(i, "\x1B[;1;4;31m"));
              return Empty;
            }
            AwaitingNum = 2;
            Oct = 1;
            Num = (Comm[i] - '0') << 6;
          }
      }
      BackSlash = 0;
    }
    else if (Comm[i] == '\\')
      BackSlash = 1;
    else if (Comm[i] == '"')
      InQuotes = !InQuotes;
    else if (Comm[i] == ' ' && !InQuotes && ArgV.back() != "")
      ArgV.push_back("");
    else
      ArgV.back() += Comm[i];
  }

  return ArgV;
}

void ExeCommand(std::vector<std::string> ArgV)
{
  if (ArgV.size() == 0)
    return;
  if (ArgV[0] == "quit")
  {
    Quit();
    return;
  }
  if (ArgV[0] == "save")
  {
    if (!File.is_open())
      Log("No file is open.");
    else
    {
      Open();
      Redraw();
    }
    return;
  }
  if (ArgV[0] == "write")
  {
    if (ArgV.size() == 1)
    {
      WriteMode = 1;
      Log("Write mode on.  Scrolling past EOF will add bytes.");
      return;
    }

    if (ArgV.size() > 2)
    {
      Log("\"write\" takes 0 or 1 arguement(s).");
      return;
    }

    if (ArgV[1] == "on")
    {
      WriteMode = 1;
      Log("Write mode on.  Scrolling past EOF will add bytes.");
      return;
    }

    if (ArgV[1] == "off")
    {
      WriteMode = 0;
      Log("Write mode off.");
      return;
    }

    Log("Unrecognized argument(s) to command");
    return;
  }
  if (ArgV[0] == "redraw")
  {
    if (ArgV.size() == 1)
    {
      //Log("Performing redraw...");
      //Sleep(400);
      Redraw();
      return;
    }

    Log("Unrecognized argument(s) to command");
    return;
  }
  if (ArgV[0] == "open")
  {
    if (ArgV.size() != 2)
    {
      Log("Usage: open <FilePath>");
      return;
    }

    FilePath = ArgV[1];
    Open();

    /*Log(std::string("0: File is ") + ((bool)File ? "" : "not ") + "good");
    Sleep(300);//*/

    Redraw();

    return;
  }
  /*if (ArgV[0] == "$" || ArgV[0] == "sys" || ArgV[0] == "/")
  {
    if (ArgV.size() == 1)
    {
      Log(":Cannot perform system call with no argument.");
      return;
    }

    std::string Command = "";
    for (int i = 1; i < ArgV.size(); i++)
    {
      Command += ArgV[i] + " ";
    }
    Command.erase(Command.length() - 1);

    //Log("Sys-call: \"" + NVCtoHexRep(Command) + "\"");
    //Sleep(1000);

    Log(":");
    system(Command.c_str());
    return;
  }//*/
  if (ArgV[0] == "filesize")
  {
    if (ArgV.size() != 1)
    {
      Log(":filesize takes no arguments");
      return;
    }

    if (!File.is_open())
    {
      Log(":No file is open");
      return;
    }

    llint Prefix = MCL::flog(1024, FileSize);
    std::string Unit = "";
    switch (Prefix)
    {
      case 1:
        Unit = "ki";
        break;
      case 2:
        Unit = "Mi";
        break;
      case 3:
        Unit = "Gi";
        break;
      case 4:
        Unit = "Ti";
        break;
      case 5:
        Unit = "Ei";
        break;
      default:
        Prefix = 0;
    }
    Unit += "B";
    double ShowFileSize = (double)FileSize / MCL::pow(1024.0, Prefix);

    Log(":" + UItoS(ShowFileSize) + Unit);
    return;
  }
  if (ArgV[0] == "winsize")
  {
    if (ArgV.size() != 1)
    {
      Log(":winsize takes no arguments");
      return;
    }

    Log(":" + UItoS(WinHeight) + "x" + UItoS(WinWidth));
    return;
  }
  if (ArgV[0] == "goto")
  {
    if (ArgV.size() != 2)
    {
      Log(":Usage `goto <Address>`");
      return;
    }

    addr Addr;
    switch (StoI(ArgV[1], Addr, 16))
    {
      case 0:
        Seek(Addr);
        return;
      case 1:
        Log(":Address must be a hexadecimal number.");
        return;
      case 2:
        Log(":Address too large for this program.");
        return;
    }
  }
  if (ArgV[0] == "find")
  {
    if (!File.is_open())
    {
      Log("No file is open");
      return;
    }

    if (ArgV.size() == 1 || ArgV.size() > 3)
    {
      Log("Usage: find <occurance> [<byte sequence>]");
      return;
    }

    int Occ;
    switch (StoI(ArgV[1], Occ))
    {
      case 1:
        Log("occurance argument must be a natural number.");
        return;
      case 2:
        Log("occurance argument is too large.");
        return;
    }

    if (ArgV.size() == 2)
    {
      if (FindAddrs.size() == 0)
      {
        Log("Nothing was previously found.  Enter something to find.");
        return;
      }
      if (Occ >= FindAddrs.size())
      {
        Log("There were only " + UItoS(FindAddrs.size()) + " occurances found (You requested " + UItoS(Occ) + ").  Here's the last one.");
        Occ = FindAddrs.size() - 1;
      }

      Seek(FindAddrs[Occ]);
      return;
    }

    Log("Looking...");

    FindAddrs.resize(0);

    const std::string Seq = ArgV[2];
    for (addr i = 0; i + Seq.length() <= FileSize; i++)
    {
      File.seekg(i);
      bool Match = 1;
      for (int j = 0; j < Seq.length(); j++)
      {
        if (Seq[j] != File.get())
        {
          Match = 0;
          break;
        }
      }
      if (Match)
      {
        FindAddrs.push_back(i);
      }
    }

    if (FindAddrs.size())
    {
      if (Occ >= FindAddrs.size())
      {
        Log("There were only " + UItoS(FindAddrs.size()) + " occurances found (You requested " + UItoS(Occ) + ").  Here's the last one.");
        Occ = FindAddrs.size() - 1;
      }
      else
        Log("Found " + UItoS(FindAddrs.size()) + " matches.  Showing match " + UItoS(Occ));

      Seek(FindAddrs[Occ]);
    }
    else
      Log("Found no occurances of \"" + Seq + "\".");
    return;
  }

  Log("Unrecognized command");
}

int GetCH()
{
  int c = _getch();
  if (c == 0x03)
    Quit();

  return c;
}

std::string CharReplace(byte c)
{
  if (' ' < c && c < 127)
  {
    return "\x1B[m" + std::string(1, c) + "\x1B[m";
  }
  if (c > 127)
  {
    return "\x1B[1;31m.\x1B[m";
  }

  std::string Output = "\x1B[36m";
  switch (c)
  {
    case 0x07:
      Output += 'a';
      break;
    case 0x08:
      Output += 'b';
      break;
    case 0x09:
      Output += 't';
      break;
    case 0x0A:
      Output += 'n';
      break;
    case 0x0B:
      Output += 'v';
      break;
    case 0x0D:
      Output += 'r';
      break;
    case 0x1B:
      Output += 'e';
      break;
    case 0x20:
      Output += '+';
      break;
    case 0x7F:
      Output += 'd';
      break;

    default:
      return "\x1B[1;31m.\x1B[m";
  }

  Output += "\x1B[m";
  return Output;
}

void DeleteByte()
{
  char TempName[L_tmpnam] = "\0";
  int Tries = 0;
  const int MaxTries = 100;
  for ( ; tmpnam(TempName) == 0 && Tries < MaxTries; Tries++) { }
  if (Tries == MaxTries)
  {
    Log("Could not create temporary file");
    return;
  }
  std::string TempPath = TempName;
  if (TempPath[0] == '\\' || TempPath[0] == '/')
    TempPath.erase(0, 1);
  std::fstream Temp(TempPath, std::ios_base::binary | std::ios_base::out);
  if (!Temp.is_open())
  {
    Log("Could not open temporary file");
    return;
  }
  Temp.seekp(0);
  File.seekg(0);
  addr i = 0;
  for ( ; i < SelAddr; i++)
  {
    Temp.put(File.get());
  }
  File.seekg(++i);
  for ( ; i < FileSize; i++)
  {
    Temp.put(File.get());
  }
  addr TempFileSize = Temp.tellp();

  File.close();
  //system(("start notepad " + TempPath).c_str());
  Temp.close();
  //system(("start notepad " + TempPath).c_str());

  remove(FilePath.c_str());
  if (rename(TempPath.c_str(), FilePath.c_str()) != 0)
  {
    Temp.open(TempPath, std::ios_base::binary | std::ios_base::in);
    if (!Temp.is_open())
    {
      Log("Couldn't open temp file");
      return;
    }
    std::ofstream New(FilePath, std::ios_base::binary | std::ios_base::out);
    if (!New.is_open())
    {
      Log("Couldn't open file");
      return;
    }

    for (byte B = Temp.get(); (bool)Temp; B = Temp.get())
      New.put(B);

    Temp.close();
    New.close();
  }
  Open();
  Seek(SelAddr);
  return;
}

void Open()
{
  if (File.is_open())
    File.close();
  File.clear();
  File.open(FilePath, std::ios_base::binary | std::ios_base::in | std::ios_base::out);
  if (!File.is_open())
  {
    Log("Could not open file!");
    return;
  }
  FindAddrs.resize(0);

  File.seekg(0, std::ios_base::end);
  FileSize = File.tellg();
  File.seekg(0);

  /*Log(std::string("2: File is ") + ((bool)File ? "" : "not ") + "good");
  Sleep(300);//*/

  AddrChars = MCL::flog(16, FileSize) + 1;
  if (AddrChars % 2)
    ++AddrChars;

  BytesPerLine = (WinWidth - AddrChars - 7) / 4;  //2 + AddrChars + 3 + 3 * BytesPerLine + 2 + BytesPerLine <= Width
  DataStart = 2 + AddrChars + 3;
  ASCIIStart = DataStart + 3 * BytesPerLine + 2;
}
