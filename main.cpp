#include <iostream>
#include <MyHeaders/strmanip.h>
#include <windows.h>
#include <conio.h>
#include <fstream>
#include <MyHeaders/Math.h>
#include <vector>


typedef unsigned char uchar;


int Width;
int Height;

std::fstream File;
unsigned long int FileSize;
int AddrChar;
int Bpl;

std::vector<std::string> TextCache;
int PosX = 0;
int PosY = 0;

int SelB = 0;

int StartAddr = 0;

bool WriteMode = 0;

void GetSize();
void Write(int x, int y, std::string msg);
void Write(std::string msg);
void Write(int x, int y, char c);
void Write(char c);

void DisplayData(int StartAddress);

void Log(std::string msg);

unsigned short int strcmp(const char* s0, const char* s1, unsigned int count);

void Highlight(int x, int y);

void Underline(int x, int y);

std::string MvCur(int x, int y);

void ResetLine(int y);

void ClrLog();

void ClrUsrBox();

char ToUpper(char c);


//Each line looks like "0x[Addr] | [HexData] | [ASCII]";  Len = 2 + AddrChar + 3 + 3 * Bpl + 2 + Bpl => Bpl = (Len - 7 - AddrChar) / 4
int main(int argc, char** argv)
{
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);

  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);



  std::string FileName;

  if (argc == 1)
  {
    std::cout << "Enter filepath: ";
    getline(std::cin, FileName);
  }
  else if (argc == 2)
  {
    FileName = argv[1];
  }
  else
  {
    std::cerr << "Usage: HexEditor [<Filepath>]\n\n";
    return 1;
  }

  Start:;
  TextCache.resize(0);

  File.open(FileName, std::ios_base::binary | std::ios_base::in | std::ios_base::out);
  if (!File.is_open())
  {
    std::cerr << "Could not open \"" << FileName << "\"\n\n";
    return 2;
  }




  GetSize();


  Write(0, Height-4, std::string(Width, '='));
  Write(0, Height-2, std::string(Width, '-'));
  //Write(0, Height-1, "\x1B[45m");
  //Write(Width/2 - 1, Height-1, "\x1B[31;1m||\x1B[0m");



  File.seekg(0, std::ios_base::end);
  FileSize = File.tellg();
  File.seekg(0);
  AddrChar = MCL::flog(16, FileSize) + 1;
  if (AddrChar % 2)
    ++AddrChar;

  Bpl = (Width - 7 - AddrChar) / 4;


  DisplayData(StartAddr);

  int Begin = 2 + AddrChar + 3;

  auto HLByte = [&](int ByteAddr){
    ByteAddr -= StartAddr;
    int UsrX = ByteAddr % Bpl;
    int UsrY = ByteAddr / Bpl;
    Highlight(3 * UsrX + Begin, UsrY);
    Highlight(3 * UsrX + 1 + Begin, UsrY);

    Underline(AddrChar + Bpl * 3 + 5 + 2 + UsrX, UsrY);
    //Log("Highlighting char at " + ItoS((ullint)(AddrChar + Bpl * 3 + 5 + 2)));
  };

  auto GetHigit = [](char c) -> int{
    if ('0' <= c && c <= '9')
      return c - '0';
    else if ('a' <= c && c <= 'f')
      return c - 'a' + 10;
    else if ('A' <= c && c <= 'F')
      return c - 'A' + 10;
    else
      return -1;
  };

  HLByte(SelB);

  int c;
  Write(0, Height-1, "");
  while(1)
  {
    c = _getch();

    if (c == 0x03)
    {
      break;
    }
    if (c == '/')
    {
      Log("Enter command");
      ClrUsrBox();
      Write(0, Height-1, '/');

      std::string Comm = "";
      c = _getch();
      while (c != 0x0D)
      {
        switch(c)
        {
          case '\b':
            Comm.erase(Comm.length() - 1, 1);
            Write("\b \b");
            break;
          default:
            Comm += (char)c;
            Write(c);
        }
        c = _getch();
      }


      if (Comm == "quit")
        break;
      else if (Comm == "save")
      {
        File.close();
        goto Start;
      }
      else if (Comm == "write on")
      {
        Log("Now in write mode.  Scrolling past EOF will add bytes.");
        WriteMode = 1;
      }
      else if (Comm == "write off")
      {
        Log("Write mode off.");
        WriteMode = 0;
      }
      else if (Comm.substr(0, 5) == "goto ")
      {
        int Addr = 0;
        switch (StoI(Comm.substr(5), Addr, 16))
        {
          case 1:
            Log("\"" + Comm.substr(5) + "\" is not a valid argument.");
            break;
          case 2:
            Log("\"" + Comm.substr(5) + "\" is too large.");
            break;
          case 0:
            StartAddr = (Addr / Bpl) * Bpl;
            DisplayData(StartAddr);
            SelB = Addr;
            HLByte(SelB);
            break;
        }
      }
      else if (Comm == "redraw")
      {
        File.close();
        goto Start;
      }
      else if (Comm.substr(0, 3) == "tb ")
      {
        std::string Call = "tb " + Comm.substr(3) + " > tmp.txt";
        Log(":");
        system(Call.c_str());
        system("cat tmp.txt");
        system("rm tmp.txt");
      }
      else if (Comm == "filesize")
      {
        Log(":" + ItoS((ullint)FileSize));
      }
      else if (Comm.substr(0, 5) == "open ")
      {
        File.close();
        FileName = Comm.substr(5);
        goto Start;
      }
      else if (Comm.substr(0, 2) == "$ ")
      {
        Log("");
        system(Comm.substr(2).c_str());
      }
      else if (Comm.substr(0, 4) == "sys ")
      {
        Log("");
        system(Comm.substr(4).c_str());
      }
      else if (Comm == "filepath")
      {
        Log(":" + FileName);
      }

      ClrUsrBox();
    }
    else if (c == 0xE0)
    {
      c = _getch();
      ClrUsrBox();
      //Write(0, Height-1, "[Arrow]");
      switch(c)
      {
        case 0x48:
          //Write('^');
          ResetLine((SelB - StartAddr) / Bpl);
          if (SelB >= Bpl)
            SelB -= Bpl;
          else
            SelB = 0;


          if (SelB < StartAddr)
          {
            //Sleep(1000);
            //Log("Selected byte is off screen.  Scrolling to " + ItoS((ullint)(StartAddr - Bpl), 16, AddrChar));
            StartAddr -= Bpl;
            DisplayData(StartAddr);
          }

          HLByte(SelB);

          //Sleep(1500);
          //Log("Byte " + ItoS((ullint)SelB, 16, 2) + " has been highlighted.");
          break;
        case 0x4B:
          //Write('<');
          ResetLine((SelB - StartAddr) / Bpl);
          if (SelB > 0)
            --SelB;

          if (SelB < StartAddr)
            DisplayData(--StartAddr);

          HLByte(SelB);
          break;
        case 0x4D:
          //Write('>');
          ResetLine((SelB - StartAddr) / Bpl);
          if (SelB == FileSize - 1)
          {
            if (WriteMode)
            {
              File.seekp(SelB);
              File.put('\0');
              File.close();
              ++SelB;

              if ((SelB - StartAddr) / Bpl >= Height - 4)
              {
                StartAddr += Bpl;
              }

              goto Start;
            }
          }
          else
            ++SelB;

          if ((SelB - StartAddr) / Bpl >= Height - 4)
          {
            StartAddr += Bpl;
            DisplayData(StartAddr);
          }

          HLByte(SelB);
          break;
        case 0x50:
          //Write('v');
          ResetLine((SelB - StartAddr) / Bpl);
          if (SelB + Bpl < FileSize)
          {
            SelB += Bpl;
          }
          else
          {
            SelB = FileSize - 1;
          }

          if ((SelB - StartAddr) / Bpl >= Height - 4)
          {
            StartAddr += Bpl;
            DisplayData(StartAddr);
          }

          HLByte(SelB);
          break;
      }
      Log("@0x" + ItoS((ullint)SelB, 16, 2));
    }
    else if (WriteMode)
    {
      char Byte = 0;
      c = ToUpper(c);
      Log("Taking write val 1/2");
      while (GetHigit(c) == -1)
      {
        c = _getch();
        c = ToUpper(c);
      }
      Byte += GetHigit(c) << 4;

      c = _getch();
      c = ToUpper(c);
      Log("Taking write val 2/2");
      while (GetHigit(c) == -1)
      {
        c = _getch();
        c = ToUpper(c);
      }
      Byte += GetHigit(c);

      //Log("Writing 0x" + ItoS((ullint)(uchar)Byte, 16, 2));
      //Sleep(1000);
      File.seekp(SelB);
      File.put(Byte);
      //Log("Seeking");
      //File.seekg(SelB);
      //Log("Fetching value");
      //Sleep(1000)
      //Log("Value: 0x" + ItoS((ullint)File.get(), 16, 2));
      //Sleep(1000);
      File.close();

      goto Start;
    }
  }

  File.close();

  Write(0,0,"\x1B[J");

  return 0;
}



void GetSize()
{
  std::string Report;

  int Old = 0;
  Width = 1;
  char c;
  while (Width > Old)
  {
    Old = Width;
    std::cout << " \x1B[6n";
    Report = "";
    c = _getch();
    while (c != 'R')
    {
      Report += c;
      c = _getch();
    }
    StoI(Report.substr(Report.find(';') + 1), Width);
  }
  Width = Old;

  Old = 0;
  Height = 1;
  while (Height > Old)
  {
    Old = Height;
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
    StoI(Report.substr(Report.find('[') + 1), Height);
  }

  std::cout << "\x1B[1;1H\x1B[J";
}



void Write(int x, int y, std::string msg)
{
  while(TextCache.size() <= y)
  {
    TextCache.push_back("");
  }

  while(TextCache[y].length() < x + msg.length())
  {
    TextCache[y] += " ";
  }

  for (int i = 0; i < msg.length(); i++)
  {
    TextCache[y][x + i] = msg[i];
  }

  std::cout << "\x1B[" + ItoS((ullint)y + 1) + ";" + ItoS((ullint)x + 1) + "H" + msg;

  PosX = x + msg.length();
  PosY = y;
}
void Write(std::string msg)
{
  while(TextCache[PosY].length() < PosX + msg.length())
  {
    TextCache[PosY] += " ";
  }

  for (int i = 0; i < msg.length(); i++)
  {
    TextCache[PosY][PosX + i] = msg[i];
  }

  std::cout << msg;

  ++PosX;
}
void Write(int x, int y, char c)
{
  while(TextCache.size() <= y)
  {
    TextCache.push_back("");
  }

  while(TextCache[y].length() < x + 1)
  {
    TextCache[y] += " ";
  }

  TextCache[y][x] = c;

  std::cout << "\x1B[" + ItoS((ullint)y + 1) + ";" + ItoS((ullint)x + 1) + "H" + c;

  PosX = x + 1;
  PosY = y;
}
void Write(char c)
{
  while(TextCache[PosY].length() < PosX + 1)
  {
    TextCache[PosY] += " ";
  }

  TextCache[PosY][PosX] = c;

  std::cout << c;

  ++PosX;
}



void DisplayData(int StartAddress)
{
  File.seekg(StartAddress);

  std::string Output;
  std::string ASCIIVal = "";
  char c = '\0';
  for (int i = 0; i < Height - 4; i++)
  {
    Output = "0x" + ItoS((ullint)(StartAddress + i * Bpl), 16, AddrChar) + " | ";
    ASCIIVal = "";
    for (int j = StartAddress + i * Bpl; j < StartAddress + (i+1)*Bpl; j++)
    {
      c = File.get();
      //Log("Addr:" + ItoS((ullint)j, 16, 2) + "; " + ItoS((ullint)(unsigned char)c, 16, 2));
      if (!File)
      {
        break;
      }
      Output += ItoS((ullint)(unsigned char)c, 16, 2) + " ";
      if (c >= ' ')
        ASCIIVal += c;
      else
        ASCIIVal += '*';//"\x1B[31;1m.\x1B[m";
    }
    Write(0, i, Output + std::string(Width - Output.length(), ' '));
    Write(AddrChar + Bpl * 3 + 5, i, "| " + ASCIIVal); //2 + AddrChar + 3 + Bpl * 3
    if (!File)
    {
      for (++i; i < Height - 4; i++)
      {
        Write(0, i, std::string(Width, ' '));
      }
    }
  }
}



void Log(std::string msg)
{
  Write(0, Height - 3, msg + "\x1B[K");
}



unsigned short int strcmp(const char* s0, const char* s1, unsigned int count)
{
  for (unsigned int i = 0; i < count; i++)
  {
    if (s0[i] < s1[i])
      return -1;
    if (s0[i] > s1[i])
      return 1;
  }
  return 0;
}



void Highlight(int x, int y)
{
  /*
  std::string Temp = TextCache[y];
  Temp.insert(x+1, "\x1B[0m");
  Temp[y].insert(x, "\x1B[1;4;100m");
  Write(0, y, Temp);*/
  int i = -1;
  char c = 0;
  bool InANSI = 0;
  for (int j = 0; i < x; j++)
  {
    c = TextCache[y][j];

    if (c == '\x1B')
      InANSI = 1;
    else if (InANSI)
    {
      if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
        InANSI = 0;
    }
    else
      ++i;

    if (i == x)
      break;
  }
  //Log("Writing \"\\x1B[4m\" + (char)0x" + ItoS((ullint)c, 16, 2) + " + \"\\x1B[m\"");
  std::cout << MvCur(x, y) + "\x1B[1;4;100m" + c + "\x1B[m" + MvCur(0, Height-1);
}

void Underline(int x, int y)
{
  int i = -1;
  char c = 0;
  bool InANSI = 0;
  for (int j = 0; i < x; j++)
  {
    c = TextCache[y][j];

    if (c == '\x1B')
      InANSI = 1;
    else if (InANSI)
    {
      if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'))
        InANSI = 0;
    }
    else
      ++i;

    if (i == x)
      break;
  }
  //Log("Writing \"\\x1B[4m\" + (char)0x" + ItoS((ullint)c, 16, 2) + " + \"\\x1B[m\"");
  //if ()
  std::cout << MvCur(x, y) + "\x1B[4m" + c + "\x1B[m" + MvCur(0, Height-1);
}

std::string MvCur(int x, int y)
{
  return "\x1B[" + ItoS((ullint)y+1) + ";" + ItoS((ullint)x+1) + "H";
}

void ResetLine(int y)
{
  Write(0, y, TextCache[y]);
}

void ClrLog()
{
  Log("");
}

void ClrUsrBox()
{
  Write(0, Height-1, std::string(Width/2 - 1, ' '));
  std::cout << MvCur(0, Height-1);
}

char ToUpper(char c)
{
  if ('a' <= c && c <= 'z')
    return c - 'a' + 'A';
  return c;
}
