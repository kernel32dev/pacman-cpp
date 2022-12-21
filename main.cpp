#if defined(UNICODE) && !defined(_UNICODE)
    #define _UNICODE
#elif defined(_UNICODE) && !defined(UNICODE)
    #define UNICODE
#endif

#include <tchar.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <windows.h>

#include <mmsystem.h>

#include <irrKlang.h>
using namespace irrklang;
using namespace std;

char BackSoundState = 4;

LARGE_INTEGER CTime; // Expected Current Time //
LONGLONG Frequency;

void ResetCT() {QueryPerformanceCounter(&CTime);}

bool KillMainLoop = false;
bool Pause = false;
bool CheatMode = false;

char SrcPath[260] = {};
char Path[260] = {};
int PathEnd;
int Zoom = 2;

void SetFilePath(char RelativeFilePath[]) {
    int Z;int X = 0;
    for (Z = PathEnd;RelativeFilePath[X] != 0;Z++) {SrcPath[Z] = RelativeFilePath[X];X++;}
    for (;Z < 260;Z++) {SrcPath[Z] = 0;}
    GetShortPathNameA(SrcPath,Path,260);
}

int GetStoredHiScore() {
    ifstream F;int Value = 0;
    SetFilePath("HiScore.txt");
    F.open(Path);
    F >> Value;
    F.close();
    return Value;
}

void SetStoredHiScore(int Value) {
    if (CheatMode) {return;}
    ofstream F;
    SetFilePath("HiScore.txt");
    F.open(Path,ios::trunc);
    F << Value;
    F.close();
}

bool DoEvents() { // Returns False If Its Time To Die //
    MSG messages;
    while (PeekMessage (&messages, NULL, 0, 0, 1))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
    return !KillMainLoop;
}

ISoundEngine* SoundEngine = createIrrKlangDevice();
ISoundEngine* BackSoundEngine = createIrrKlangDevice();

class Sound {
    bool M;
    bool Initialized;
public:
    bool IsSoundOn;
    char CurrentBackGround;
    void Terminate() {if(!Initialized){return;} SoundEngine->drop(); Initialized = false;/*Delete Engine*/};
    void Munch () { if (!IsSoundOn) {return;}
        if (M) {SetFilePath("assets\\sounds\\M1.wav");}
        else {SetFilePath("assets\\sounds\\M2.wav");}
        SoundEngine->play2D(Path, false);M =! M;
    };
    void Death () {if (!IsSoundOn) {return;}SetFilePath("assets\\sounds\\D.wav");SoundEngine->play2D(Path, false);};
    void GameStart () {if (!IsSoundOn) {return;}SetFilePath("assets\\sounds\\GS.wav");SoundEngine->play2D(Path, false);};
    void EatFruit () {if (!IsSoundOn) {return;}SetFilePath("assets\\sounds\\EF.wav");SoundEngine->play2D(Path, false);};
    void EatGhost () {if (!IsSoundOn) {return;}SetFilePath("assets\\sounds\\EG.wav");SoundEngine->play2D(Path, false);};
    void BackGround (char Id) {
        if (Id != CurrentBackGround) {
            BackSoundEngine->stopAllSounds();
            if (Id == 0) {if (!IsSoundOn) {return;}SetFilePath("assets\\sounds\\BS0.wav");BackSoundEngine->play2D(Path, true);}
            else if (Id == 1) {if (!IsSoundOn) {return;}SetFilePath("assets\\sounds\\BS1.wav");BackSoundEngine->play2D(Path, true);}
            else if (Id == 2) {if (!IsSoundOn) {return;}SetFilePath("assets\\sounds\\BS2.wav");BackSoundEngine->play2D(Path, true);}
            else {Id = 3;}
            CurrentBackGround = Id;
        }
    };
    Sound () {
        Initialized = (bool)SoundEngine;
        CurrentBackGround = 3;
        IsSoundOn = true;
    }
}Sound;

const char FruitTable[] = {0,1,2,2,3,3,4,4,5,5,6,6,7,7,7,7,7,7};
const int FruitScoreTable[8] = {100,300,500,700,1000,2000,3000,5000};
int FruitId = 0;
bool Fruit = false;

long GameTick = 0;
long LastPelletEaten = 0; // Time
int Score = 0;
int HiScore;
int OldHiScore;
int PelletsEaten = 0;
int Level = 0;
int Lives = 0;
bool HasDied = false;

HDC hdc = 0;

class Buffer {
    HBITMAP Old_Bmp;
public:
    HDC Hdc;
    HBITMAP Bmp;
    int Width;
    int Height;
    void SetSize(int NewWidth,int NewHeight);
    void Destroy();
    void Print(HDC hdc,int X,int Y);
    void Clear(COLORREF Color);
    bool LoadFromFile(LPCSTR Path,int NewWidth,int NewHeight);
};
void Buffer::SetSize(int NewWidth,int NewHeight) {
Destroy();
Width=NewWidth;
Height=NewHeight;
HDC DisplayHdc = CreateDCA(_T("DISPLAY"),nullptr,nullptr,nullptr);
Hdc = CreateCompatibleDC(DisplayHdc);
Bmp=CreateCompatibleBitmap(DisplayHdc,Width,Height);
Old_Bmp = (HBITMAP)SelectObject(Hdc,(HGDIOBJ)Bmp);
DeleteDC(DisplayHdc);
}
void Buffer::Destroy() {
    if (Hdc != 0) {
        if (Old_Bmp != 0) {DeleteObject(SelectObject(Hdc,Old_Bmp));}
        DeleteDC(Hdc);Hdc = 0;
    } Old_Bmp = 0;Bmp = 0;
}
void Buffer::Print(HDC DestHdc,int X,int Y) {
BitBlt(DestHdc,X,Y,Width,Height,Hdc,0,0,SRCCOPY);
}
void Buffer::Clear(COLORREF Color) {
if (Hdc == 0) {return;}
HBRUSH Brush = CreateSolidBrush(Color);
RECT R;
R.left = 0;
R.top = 0;
R.right = Width;
R.bottom = Height;
FillRect(Hdc,&R,Brush);
DeleteObject(Brush);
}
bool Buffer::LoadFromFile(LPCSTR Path,int NewWidth,int NewHeight) {


//HBITMAP TempBmp = (HBITMAP)LoadImage(NULL,Path,0,0,0,LR_LOADFROMFILE);
//HANDLE TempBmp = LoadImageA(0,"C:\Warehouse\Projetos C++\Win32Gui\assets\Mono.bmp" + NC + NC,0,0,0,16);
//char TXT[260] = "C:\\Warehouse\\Projetos C++\\Win32Gui\\assets\\Mono.bmp";
//TXT[50] = 0;

//std::cout << TXT;
HANDLE TempBmp = LoadImageA(0,Path,0,0,0,16);
//HANDLE TempBmp = LoadImageA(0,TXT,0,0,0,16);
if (TempBmp == 0) {return false;}
Destroy();Bmp = (HBITMAP)TempBmp;
HDC DisplayHdc = CreateDCA(_T("DISPLAY"),nullptr,nullptr,nullptr);
Hdc = CreateCompatibleDC(DisplayHdc);
Old_Bmp = (HBITMAP)SelectObject(Hdc,(HGDIOBJ)Bmp);
Width=NewWidth;
Height=NewHeight;
DeleteDC(DisplayHdc);
return true;
}

Buffer SBuff; // Screen Buffer, BBuff Streched
Buffer BBuff; // Where Sprites and Grid Will Be Drawn
Buffer TBuff; // Texture Buffer
Buffer NotTBuff; // Texture Buffer Inverted
Buffer TextBuff; // Text Texture Buffer
Buffer FruitsBuff; // Fruits Texture Buffer
Buffer GBuff; // Grid Buffer
Buffer TempBuff; // Temporary Buffer 16x16
Buffer FTempBuff; // Secondary Temporary Buffer 20x7

const int FDX[4] = {0,1,0,-1}; // To Translate Facing Direction
const int FDY[4] = {-1,0,1,0};

struct Keys{
bool Up = false;
bool Down = false;
bool Left = false;
bool Right = false;
}Keys;

void FillSolidRect(HDC DestHdc,int X,int Y,int W,int H,COLORREF Color) {
HBRUSH Brush = CreateSolidBrush(Color);
RECT R;
R.top = Y;
R.left = X;
R.bottom = Y + H;
R.right = X + W;
FillRect(DestHdc,&R,Brush);
DeleteObject(Brush);
}
void FillBrushRect(HDC DestHdc,int X,int Y,int W,int H,HBRUSH Brush) {
RECT R;
R.top = Y;
R.left = X;
R.bottom = Y + H;
R.right = X + W;
FillRect(DestHdc,&R,Brush);
}

void DrawSprite(HDC DestHdc,int X, int Y, int SrcX, int SrcY, COLORREF Color) {
BitBlt(DestHdc,X,Y,16,16,NotTBuff.Hdc,SrcX,SrcY,SRCAND); TempBuff.Clear(Color);
BitBlt(TempBuff.Hdc,0,0,16,16,TBuff.Hdc,SrcX,SrcY,SRCAND);
BitBlt(DestHdc,X,Y,16,16,TempBuff.Hdc,0,0,SRCPAINT);
}

void ShowPoints(int X, int Y, int PointsIndex) {
    GBuff.Print(BBuff.Hdc,0,0);
    if (PointsIndex < 4) {
        FillSolidRect(BBuff.Hdc,X,Y,20,7,RGB(1,255,255));
        Sound.EatGhost();
    } else {
        FillSolidRect(BBuff.Hdc,X,Y,20,7,RGB(255,135,255));
        Sound.EatFruit();
    } BitBlt(BBuff.Hdc,X,Y,20,7,TextBuff.Hdc,PointsIndex * 20,16,SRCAND);
    StretchBlt(hdc,0,0,SBuff.Width,SBuff.Height,BBuff.Hdc,0,0,BBuff.Width,BBuff.Height,SRCCOPY);
    Sleep(500);ResetCT();
}

void DrawNumber(int Number,int X,int Y,COLORREF Color,char Alingment) {
HBRUSH B = CreateSolidBrush(Color);
         if (Alingment == 0) {for (int Z = Number / 10;Z != 0;Z /= 10) {X += 8;};}
    else if (Alingment == 1) {for (int Z = Number / 10;Z != 0;Z /= 10) {X += 4;}X -= 4;}
    if (Number != 0) {
        do {
            FillBrushRect(GBuff.Hdc,X,Y,8,8,B);
            BitBlt(GBuff.Hdc,X,Y,8,8,TextBuff.Hdc,(Number % 10)*8,8,SRCAND);
            Number /= 10;
            X -= 8;
        } while (Number != 0);
    } else {
        FillBrushRect(GBuff.Hdc,X,Y,8,8,B);
        BitBlt(GBuff.Hdc,X,Y,8,8,TextBuff.Hdc,0,8,SRCAND);
    }
DeleteObject((HGDIOBJ)B);
}

HDC DrawStringDestHdc = 0;
void DrawString(char Text[], int TextLen ,int X,int Y,COLORREF Color,char Alingment) {
    if (DrawStringDestHdc == 0) {DrawStringDestHdc = GBuff.Hdc;}
char C; HBRUSH B = CreateSolidBrush(Color);HBRUSH BackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
         if (Alingment == 2) {X -= 8 * TextLen;}
    else if (Alingment == 1) {X -= 4 * TextLen;}
    for (int Z=0;Z < TextLen;Z++) {
        C = Text[Z];
        FillBrushRect(DrawStringDestHdc,X,Y,8,8,B);
        if (C >= 65 && C <= 90) {
            BitBlt(DrawStringDestHdc,X,Y,8,8,TextBuff.Hdc,(C - 65)*8,0,SRCAND);
        } else if (C == 33) {
            BitBlt(DrawStringDestHdc,X,Y,8,8,TextBuff.Hdc,80,8,SRCAND);
        } else {
            FillBrushRect(DrawStringDestHdc,X,Y,8,8,BackBrush);
        }
        X += 8;
    }
DeleteObject((HGDIOBJ)B);
}
/* Alingment
 * Left   = 0
 * Middle = 1
 * Right  = 2
 */
class Grid {
public:
    bool DoMirroring = false;
    COLORREF GridColor = 0xFFFFFF;
    HDC DestHdc;
    const int Width = 28;
    const int Height = 31;
    void SetBlock(char Value,int X,int Y);
    char GetBlock(int X,int Y);
private:
    char G[28][31];
    /* 0 = Pellet
     * 1 = Power Pellet
     * 2 = Empty
     * 3 = Ghost House
     * 4 = Solid */
} Grid;
void Grid::SetBlock(char Value,int X,int Y) {
    if (Value < 4) {
        G[X][Y] = Value;
        BitBlt(DestHdc,X*8,Y*8+24,8,8,TBuff.Hdc,(Value & 252)*2,(Value & 3)*8,SRCCOPY);
    } else if (Value != 127) {
        G[X][Y] = 4;
        FillSolidRect(DestHdc,X*8,Y*8+24,8,8,GridColor);
        BitBlt(DestHdc,X*8,Y*8+24,8,8,TBuff.Hdc,(Value & 252)*2,(Value & 3)*8,SRCAND);
    } else { // Value = 127
        G[X][Y] = 4;
        FillSolidRect(DestHdc,X*8,Y*8+24,8,8,0);
    }
}
char Grid::GetBlock(int X,int Y) {
    if (X < 0) {X = 0;}
    if (Y < 0) {Y = 0;}
    if (X > 27) {X = 27;}
    if (Y > 30) {Y = 30;}
    return G[X][Y];
}

struct Pacman {
    void DrawPacman(HDC DestHdc) {
        if (AnimationFrame < 4) {
            if (AnimationFrame == 2) {DrawSprite(DestHdc, X - 4, Y + 20, 32, 32, 0x00FFFF);}
            else if (AnimationFrame == 0) {DrawSprite(DestHdc, X - 4, Y + 20, 0, 32 + FacingDirection * 16, 0x00FFFF);}
            else {DrawSprite(DestHdc, X - 4, Y + 20, 16, 32 + FacingDirection * 16, 0x00FFFF);}

            if (X >= 196) {
                X -= 224;
                if (AnimationFrame == 2) {DrawSprite(DestHdc, X - 4, Y + 20, 32, 32, 0x00FFFF);}
                else if (AnimationFrame == 0) {DrawSprite(DestHdc, X - 4, Y + 20, 0, 32 + FacingDirection * 16, 0x00FFFF);}
                else {DrawSprite(DestHdc, X - 4, Y + 20, 16, 32 + FacingDirection * 16, 0x00FFFF);}
                    X += 224;
            };
        } else {
            if (AnimationFrame <= 8) {
            DrawSprite(DestHdc, X - 4, Y + 20, 16 * (AnimationFrame - 4), 96, 0x00FFFF);
            } else if (AnimationFrame <= 13) {
            DrawSprite(DestHdc, X - 4, Y + 20, 16 * (AnimationFrame - 9), 112, 0x00FFFF);
            }
        }
    }
    void Reset() {
        X = 108;
        Y = 184;
        FacingDirection = 1;
        AnimationFrame = 2;
        Moving = false;
        Dead = false;
    }
    int X; // 8 Units per Tile
    int Y;
    int FacingDirection; // 0 to 3
    int AnimationFrame; // 0 to 3: Open Half Closed Half // 4.. Dying Animation
    bool Moving;bool Dead;
}Pacman;

int FrightenedTime = 0;
int FrightenedCombo = 0;
bool ScatterMode = true;

void AddLife(bool Value) {
    for (int Z = 0;Z < Lives;Z++) {
        FillBrushRect(GBuff.Hdc,Z*16 + 16,274,16,16,(HBRUSH)GetStockObject(BLACK_BRUSH));
    }
    if (Value) {Lives++; if (Lives > 5) {Lives = 5;}}
    else {Lives--;}
    for (int Z = 0;Z < Lives;Z++) {
        DrawSprite(GBuff.Hdc,Z*16 + 16,272,64,32,0x00FFFF);
    }
}

void AddScore(int ExtraScore) {
    if (Score / 10000 < (Score + ExtraScore) / 10000) {AddLife(true);}
    Score += ExtraScore;
    DrawNumber(Score,112,12,0xFFFFFF,1);
    if (Score > HiScore && !CheatMode) {
        HiScore = Score;
        DrawNumber(HiScore,0,12,0x10101 * 200,0);
    }
}

class Ghost {
    char CanMoveTo(bool CanGoTo3,char *Count);
public:
    int Id; // Ghost Id 0 to 3
    int X; // 8 Units per Tile
    int Y;
    int FacingDirection; // 0 to 3
    bool Frightened;
    bool Eaten;
    bool AnimationFrame;
    bool IsOnGhostHouse;
    void DrawGhost(HDC DestHdc);
    void Tick(int GoalX,int GoalY);
    void Reset();
    Ghost (int GhostId) {Id = GhostId;Reset();}
};
void Ghost::Reset() {
    X = 98 + Id * 7;
    Y = 112;
    FacingDirection = 1;
    AnimationFrame = false;
    Frightened = false;
    Eaten = false;
    IsOnGhostHouse = (Id != 0);
}
void Ghost::DrawGhost(HDC DestHdc) {
    //std::cout << X << " " << Y << std::endl;
    //DrawSprite(DestHdc,4,28,32,64,0x00FFFF); return;
    if (!Frightened) {
    COLORREF Color;
         if (Id == 0) {Color = RGB(255,0,0);}
    else if (Id == 1) {Color = RGB(255,189,255);}
    else if (Id == 2) {Color = RGB(0,255,255);}
    else if (Id == 3) {Color = RGB(255,189,81);}
    if (Eaten) {/* Dont Draw Body If Eaten*/}
    else if (AnimationFrame) {DrawSprite(DestHdc,X-4,Y+20,32,48,Color);}
    else {DrawSprite(DestHdc,X-4,Y+20,32,64,Color);}
    // RGB(225,225,255) RGB(33,33,255) EyeColor
    FillSolidRect(TempBuff.Hdc,0,0,16,8,RGB(225,225,255));
    FillSolidRect(TempBuff.Hdc,0,8,16,8,RGB(33,33,255));
    BitBlt(TempBuff.Hdc,0,0,16,16,TBuff.Hdc,32,80,SRCAND);
    int EX=X-1,EY=Y+24,IX=EX,IY=EY;
         if (FacingDirection == 0) {EY -= 2;IY -= 4;}
    else if (FacingDirection == 1) {EX += 1;IX += 2;}
    else if (FacingDirection == 2) {EY += 1;IY += 2;}
    else if (FacingDirection == 3) {EX -= 1;IX -= 2;}
    BitBlt(DestHdc,EX,EY,16,8,NotTBuff.Hdc,32,80,SRCAND);
    BitBlt(DestHdc,EX,EY,16,8,TempBuff.Hdc,0,0,SRCPAINT);
    BitBlt(DestHdc,IX,IY,16,8,NotTBuff.Hdc,32,88,SRCAND);
    BitBlt(DestHdc,IX,IY,16,8,TempBuff.Hdc,0,8,SRCPAINT);
    } else {  // Frightened Ghosts
        const int BlinkingTime = 270;
        if ((FrightenedTime % (BlinkingTime / 3)) < (BlinkingTime / 6) && FrightenedTime < BlinkingTime) {
            if (AnimationFrame) {DrawSprite(DestHdc,X-4,Y+20,32,48,RGB(222,222,255));}
            else {DrawSprite(DestHdc,X-4,Y+20,32,64,RGB(222,222,255));}
            DrawSprite(DestHdc,X-4,Y+20,48,32,RGB(255,0,0));
        } else {
            if (AnimationFrame) {DrawSprite(DestHdc,X-4,Y+20,32,48,RGB(33,33,255));}
            else {DrawSprite(DestHdc,X-4,Y+20,32,64,RGB(33,33,255));}
            DrawSprite(DestHdc,X-4,Y+20,48,32,RGB(255,183,174));
        }
    }
    if (X >= 196) {
    X -= 224;
    DrawGhost(DestHdc);
    X += 224;
    }
    //FillSolidRect(DestHdc,(X/8)*8,(Y/8)*8+24,8,8,255);
}
void Ghost::Tick(int GoalX,int GoalY) {
    if (Eaten) {
        GoalX = 108; GoalY = 112;
        if (X == GoalX && Y == GoalY) {Eaten = false;}
    }
    if ((!Frightened || GameTick % 3 != 0) && (Y != 112 || GameTick % 2 || (X >= 24 && X <= 200))) {
        X += FDX[FacingDirection];
        Y += FDY[FacingDirection];
        if (X >= 228) {X -= 224;}
        if (X < 4) {X += 224;}
        if (Y >= 292) {Y -= 288;}
        if (Y < 4) {Y += 288;}
    }
if (X % 8 == 0 && Y % 8 == 0) {
    char Move = 0;
    char MoveC = 0;
    char C = 2;
    if (Eaten) {C = 3;}
    else if (Grid.GetBlock(X/8,Y/8) == 3) {C = 3;}

    if (FacingDirection != 0 && !IsOnGhostHouse) {if (Grid.GetBlock(X/8,Y/8 + 1) <= C) {Move |= 4;MoveC++;}}
    if (FacingDirection != 1)                    {if (Grid.GetBlock(X/8 - 1,Y/8) <= C) {Move |= 8;MoveC++;}}
    if (FacingDirection != 2 && !IsOnGhostHouse) {if (Grid.GetBlock(X/8,Y/8 - 1) <= C) {Move |= 1;MoveC++;}}
    if (FacingDirection != 3)                    {if (Grid.GetBlock(X/8 + 1,Y/8) <= C) {Move |= 2;MoveC++;}}
    C = 0;
    //if (C == 3) {std::cout << "ID:" << Id << "   We Are On 3" << std::endl;}

    if (Move == 0) {
    FacingDirection += 2; // No Options, Turn Back
    FacingDirection %= 4;
    } else if (MoveC == 1) { // One Option Go There
        FacingDirection = -1;
             if (Move == 1) {FacingDirection = 0;}
        else if (Move == 2) {FacingDirection = 1;}
        else if (Move == 4) {FacingDirection = 2;}
        else if (Move == 8) {FacingDirection = 3;}
    } else {
        // Multiple Options Chose Closest To Goal
        if (Frightened) {
            // Randomize Goal To One Of The Scatter Goals
            switch ((GameTick + Id + X + Y + Pacman.X + Pacman.Y) % 4) {
                case 0: GoalX = 200; GoalY = -24; break;
                case 1: GoalX = 216; GoalY = 256; break;
                case 2: GoalX = 16; GoalY = -24; break;
                case 3: GoalX = 0; GoalY = 256; break;
            }
        }
            long D; long Best = 10000000.0;
            C = 1;FacingDirection = -1;
            for(int Z = 0;Z <= 3;Z++) {
                if (Move & C) {
                    D = (X + FDX[Z] - GoalX) * (X + FDX[Z] - GoalX) +
                        (Y + FDY[Z] - GoalY) * (Y + FDY[Z] - GoalY);
                    if (D < Best) {
                        Best = D;
                        FacingDirection = Z;
                    }
                }C *= 2;
            }

    }
} else if (X == 108 && Y == 112 && !Eaten && !IsOnGhostHouse)
{FacingDirection = 0;}
else if (X == 108 && Y == 88 && !Eaten && !IsOnGhostHouse)
{if (Pacman.X >= 108) {FacingDirection = 1;} else {FacingDirection = 3;}}
    if (!Eaten) { // Check For Collision With Player
        if (Pacman.X < X + 4 && X < Pacman.X + 4 && Pacman.Y < Y + 4 && Y < Pacman.Y + 4) {
            // Collided
            if (Frightened) {
                int ExtraPoints = 200;
                for (int Z = 0; Z < FrightenedCombo;Z++) {ExtraPoints *= 2;}
                AddScore(ExtraPoints);
                ShowPoints(X-8,Y+24,FrightenedCombo);
                Eaten = true;Frightened = false;
                if (FrightenedCombo < 3) {FrightenedCombo++;}
            } else if (!CheatMode) {/* Pacman MUST DIE */
                HasDied = true;
                Sleep(250);ResetCT();
                Fruit = false;
                Sound.Death();
                Pacman.Dead = true;
                Pacman.FacingDirection = 0;
                Pacman.AnimationFrame = 3;
                GameTick &= !1; // Make GameTick Even Number
            }
        }
    }
}

Ghost Blinky(0);
Ghost Pinky(1);
Ghost Inky(2);
Ghost Clyde(3);

int PelletCounter = 0;

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);
/*  Make the class name into a global variable  */
TCHAR szClassName[ ] = _T("C++Pacman");

void UpdateFruits(){
int Z;int X = 192;
if (Level <= 7) {for (Z = 0;Z < Level;Z++) {BitBlt(GBuff.Hdc,X,272,16,16,FruitsBuff.Hdc,FruitTable[Z] * 16,0,SRCCOPY);X -= 16;}}
else if (Level <= 18) {for (Z = Level-7;Z < Level;Z++) {BitBlt(GBuff.Hdc,X,272,16,16,FruitsBuff.Hdc,FruitTable[Z] * 16,0,SRCCOPY);X -= 16;}}
else {for (Z = 0;Z < 7;Z++) {BitBlt(GBuff.Hdc,X,272,16,16,FruitsBuff.Hdc,7 * 16,0,SRCCOPY);X -= 16;}}
}

void ResetMap(COLORREF MapColor,bool AddPellets) {
        const char Map[31][14] = {
{24,18,18,18,18,18,18,18,18,18,18,18,18,32},
{17,0,0,0,0,0,0,0,0,0,0,0,0,7},
{17,0,8,4,4,9,0,8,4,4,4,9,0,7},
{17,1,7,3,3,5,0,7,3,3,3,5,0,7},
{17,0,11,6,6,10,0,11,6,6,6,10,0,11},
{17,0,0,0,0,0,0,0,0,0,0,0,0,0},
{17,0,8,4,4,9,0,8,9,0,8,4,4,4},
{17,0,11,6,6,10,0,7,5,0,11,6,6,13},
{17,0,0,0,0,0,0,7,5,0,0,0,0,7},
{27,16,16,16,16,9,0,7,15,4,4,9,2,7},
{3,3,3,3,3,17,0,7,12,6,6,10,2,11},
{3,3,3,3,3,17,0,7,5,2,2,2,2,2},
{3,3,3,3,3,17,0,7,5,2,20,16,16,3},  //////// WAS A 3 (GHOST HOUSE DOOR)
{18,18,18,18,18,10,0,11,10,2,19,127,127,3},
{2,2,2,2,2,2,0,2,2,2,19,127,3,3},
{16,16,16,16,16,9,0,8,9,2,19,127,127,127},
{3,3,3,3,3,17,0,7,5,2,23,18,18,18},
{3,3,3,3,3,17,0,7,5,2,2,2,2,2},
{3,3,3,3,3,17,0,7,5,2,8,4,4,4},
{24,18,18,18,18,10,0,11,10,2,11,6,6,13},
{17,0,0,0,0,0,0,0,0,0,0,0,0,7},
{17,0,8,4,4,9,0,8,4,4,4,9,0,7},
{17,0,11,6,9,5,0,11,6,6,6,10,0,11},
{17,1,0,0,7,5,0,0,0,0,0,0,0,2},
{31,4,9,0,7,5,0,8,9,0,8,4,4,4},
{33,6,10,0,11,10,0,7,5,0,11,6,6,13},
{17,0,0,0,0,0,0,7,5,0,0,0,0,7},
{17,0,8,4,4,4,4,14,15,4,4,9,0,7},
{17,0,11,6,6,6,6,6,6,6,6,10,0,11},
{17,0,0,0,0,0,0,0,0,0,0,0,0,0},
{27,16,16,16,16,16,16,16,16,16,16,16,16,16}};
Grid.GridColor = MapColor;
char C;
    for (int x = 0; x < 14; x++) {
        for (int y = 0; y < 31; y++) {
            C = Map[y][x];
            if (!AddPellets && (C == 0 || C == 1)) {C = 2;}
            Grid.SetBlock(C,x,y);
            if ((C >= 8 && C < 16) || (C >= 20 && C < 28)) {
                Grid.SetBlock(C ^ 1,27-x,y);
            } else if (((C >= 4 && C < 8) || (C >= 16 && C < 20)) && C & 1) {
                Grid.SetBlock(C ^ 2,27-x,y);
            } else if (C >= 28 && C < 32) {
                Grid.SetBlock(C + 4,27-x,y);
            } else if (C >= 32 && C < 36) {
                Grid.SetBlock(C - 4,27-x,y);
            } else {
                Grid.SetBlock(C,27-x,y);
            }
        }
    }PelletsEaten = 0;
    FillSolidRect(GBuff.Hdc,103,125,1,2,Grid.GridColor);
    FillSolidRect(GBuff.Hdc,120,125,1,2,Grid.GridColor);
    FillSolidRect(GBuff.Hdc,104,125,16,2,RGB(255,185,255));
}

void ResetGame(bool AdvanceLevel) {

    Blinky.Reset();
    Inky.Reset();
    Pinky.Reset();
    Clyde.Reset();
    Pacman.Reset();

    GameTick = 0;
    LastPelletEaten = 0;
    PelletCounter = 0;
    ScatterMode = true;
    Fruit = false;
    if (AdvanceLevel) {
        HasDied = false;
        ResetMap(Grid.GridColor,true);
        PelletsEaten = 0;
        Level++;
        UpdateFruits();
    }
}

void Redraw() {
    SBuff.Clear(0);
    GBuff.Print(BBuff.Hdc,0,0);
    if (!Pacman.Dead) {
        if (Fruit) {
            BitBlt(BBuff.Hdc,104,156,16,16,FruitsBuff.Hdc,FruitId * 16,0,SRCCOPY);
        }
        if (Blinky.Eaten || Blinky.Frightened) {Blinky.DrawGhost(BBuff.Hdc);}
        if (Inky.Eaten || Inky.Frightened) {Inky.DrawGhost(BBuff.Hdc);}
        if (Pinky.Eaten || Pinky.Frightened) {Pinky.DrawGhost(BBuff.Hdc);}
        if (Clyde.Eaten || Clyde.Frightened) {Clyde.DrawGhost(BBuff.Hdc);}
    } Pacman.DrawPacman(BBuff.Hdc);
    if (!Pacman.Dead) {
        if (!(Blinky.Eaten || Blinky.Frightened)) {Blinky.DrawGhost(BBuff.Hdc);}
        if (!(Inky.Eaten || Inky.Frightened)) {Inky.DrawGhost(BBuff.Hdc);}
        if (!(Pinky.Eaten || Pinky.Frightened)) {Pinky.DrawGhost(BBuff.Hdc);}
        if (!(Clyde.Eaten || Clyde.Frightened)) {Clyde.DrawGhost(BBuff.Hdc);}
    }
    StretchBlt(SBuff.Hdc,0,0,SBuff.Width,SBuff.Height,BBuff.Hdc,0,0,BBuff.Width,BBuff.Height,SRCCOPY);
    SBuff.Print(hdc,0,0);
}

void WaitUntil(LARGE_INTEGER TimeResultTime) { // May Set KillMainLoop !!!
    MSG DummyMessage;
    LONGLONG C = TimeResultTime.QuadPart;
    QueryPerformanceCounter(&TimeResultTime);
    C -= TimeResultTime.QuadPart;
    C /= Frequency;
    // In C Now is Time To Wait
    //cout << "Waiting: " << C << "ms" << endl;
    if (C > 1) {
        UINT_PTR TimerHwnd = SetTimer(NULL,1,(UINT)C,NULL);
        if (!GetMessage(&DummyMessage,NULL,0x0113,0x0113)) {
            KillMainLoop = true;
        }
        KillTimer(NULL,TimerHwnd);
    }
}

bool MainLoop(bool * EndLoop) {
    *EndLoop = true;
        int X = 0;
        int Y = 0;
        int Z = 0;
        const int MPCount = 4;// Number Of Entries
        const unsigned char MinumumPellets[5][3] =
        {{7,17,32},{10,80,130},{8,60,125},{5,50,115},{1,30,60}};

        if (Pause) {
            BackSoundState = 4;
            Sound.BackGround(3);
            DrawStringDestHdc = BBuff.Hdc;
            DrawString("PAUSED",6,112,160,0xFFFF00,1);
            do {
                StretchBlt(hdc,0,0,SBuff.Width,SBuff.Height,BBuff.Hdc,0,0,BBuff.Width,BBuff.Height,SRCCOPY);
                Sleep(40);
                DoEvents();
            } while (Pause);
            DrawStringDestHdc = 0;
            FillBrushRect(GBuff.Hdc,88,160,48,8,(HBRUSH)GetStockObject(BLACK_BRUSH));
            ResetCT();
        }
        const int TPS = 80;
        if (Level == 1) {   // LEVEL 1 //
                 if (GameTick == (4  +  0) * TPS) {ScatterMode = false;}
            else if (GameTick == (20 +  4) * TPS) {ScatterMode = true ;}
            else if (GameTick == (7  + 24) * TPS) {ScatterMode = false ;}
            else if (GameTick == (20 + 31) * TPS) {ScatterMode = true ;}
            else if (GameTick == (5  + 51) * TPS) {ScatterMode = false;}
            else if (GameTick == (20 + 56) * TPS) {ScatterMode = true ;}
            else if (GameTick == (5  + 76) * TPS) {ScatterMode = false ;}
        } else if (Level <= 4) { // LEVEL 2 to LEVEL 4 //
                 if (GameTick == (4  +  0) * TPS) {ScatterMode = false;}
            else if (GameTick == (20 +  4) * TPS) {ScatterMode = true ;}
            else if (GameTick == (7  + 24) * TPS) {ScatterMode = false ;}
            else if (GameTick == (20 + 31) * TPS) {ScatterMode = true ;}
            else if (GameTick == (5  + 51) * TPS) {ScatterMode = false;}
        } else { // LEVEL 5 AND BEYOND //
                 if (GameTick == (2  +  0) * TPS) {ScatterMode = false;}
            else if (GameTick == (20 +  2) * TPS) {ScatterMode = true ;}
            else if (GameTick == (5  + 22) * TPS) {ScatterMode = false ;}
            else if (GameTick == (20 + 27) * TPS) {ScatterMode = true ;}
            else if (GameTick == (5  + 47) * TPS) {ScatterMode = false;}
        }

        if (FrightenedTime != 0) {FrightenedTime--;if (FrightenedTime == 0 ||
            !(Blinky.Frightened || Inky.Frightened || Pinky.Frightened || Clyde.Frightened)) {
            FrightenedCombo = 0;
            Blinky.Frightened = false;
            Inky.Frightened = false;
            Pinky.Frightened = false;
            Clyde.Frightened = false;
        }}
        if (Pacman.Dead) {BackSoundState = 3;} else if (BackSoundState == 3) {BackSoundState = 4;}
        if (BackSoundState == 3) {Sound.BackGround(3);}
        else if ((Blinky.Eaten || Inky.Eaten || Pinky.Eaten || Clyde.Eaten)) {BackSoundState = 2;Sound.BackGround(BackSoundState);}
        else if ((Blinky.Frightened || Inky.Frightened || Pinky.Frightened || Clyde.Frightened)) {BackSoundState = 1;Sound.BackGround(BackSoundState);}
        else if (BackSoundState != 0) {BackSoundState = 0;Sound.BackGround(BackSoundState);}
        if (!Pacman.Dead) {/* START OF TICKING */
                if (!HasDied) {Z = Level; if (Z > MPCount) {Z = MPCount;}}
                else {Z = 0;}
            if (Pinky.IsOnGhostHouse) {Pinky.IsOnGhostHouse = !((char)PelletCounter >= MinumumPellets[Z][0]);}
            if (Inky.IsOnGhostHouse) {Inky.IsOnGhostHouse  = !((char)PelletCounter >= MinumumPellets[Z][1]);}
            if (Clyde.IsOnGhostHouse) {Clyde.IsOnGhostHouse = !((char)PelletCounter >= MinumumPellets[Z][2]);}
        //std::cout << Pacman.X << " " << Pacman.Y << " " << Pacman.Moving << std::endl;
        if (GameTick % 4 == 0) {
            if (Pacman.Moving) {
                Pacman.AnimationFrame++;
                Pacman.AnimationFrame %= 4;
            }
        }
            if (ScatterMode) {
                Blinky.Tick(200,-24);
                Inky.Tick(216,256);
                Pinky.Tick(16,-24);
                Clyde.Tick(0,256);
            } else {
                Blinky.Tick(Pacman.X,Pacman.Y);
                X = Pacman.X + FDX[Pacman.FacingDirection] * 2;
                Y = Pacman.Y + FDY[Pacman.FacingDirection] * 2;
                Inky.Tick(2*X - Blinky.X,2*Y - Blinky.Y);
                Pinky.Tick(Pacman.X + FDX[Pacman.FacingDirection] * 4,Pacman.Y + FDY[Pacman.FacingDirection] * 4);
                if ((Pacman.X - Clyde.X)*(Pacman.X - Clyde.X)+(Pacman.Y - Clyde.Y)*(Pacman.Y - Clyde.Y) < 2034) {Clyde.Tick(0,200);}
                else {Clyde.Tick(Pacman.X,Pacman.Y);}
            }

        if (GameTick % 20 == 0) {
            Blinky.AnimationFrame = !Blinky.AnimationFrame;
            Inky.AnimationFrame = !Inky.AnimationFrame;
            Pinky.AnimationFrame = !Pinky.AnimationFrame;
            Clyde.AnimationFrame = !Clyde.AnimationFrame;
        }
            if (Pacman.Moving) {
                if (Pacman.X % 8 == 0 && Pacman.Y % 8 == 0) {
                    if (Fruit && Pacman.X / 8 == 13 && Pacman.Y / 8 == 17) {
                        // Eat Fruit
                        Fruit = false;
                        ShowPoints(104,160,FruitId + 4);
                        AddScore(FruitScoreTable[FruitId]);
                    }

                    Z = Grid.GetBlock(Pacman.X / 8 ,Pacman.Y / 8);
                    if (Z == 0 || Z == 1) {
                        Sound.Munch();
                        LastPelletEaten = GameTick;PelletsEaten++;PelletCounter++;
                        Grid.SetBlock(2, Pacman.X / 8 ,Pacman.Y / 8);
                        AddScore(10);
                        if (PelletsEaten == 244) {

                            const long SleepTime = 300;

                            BackSoundState = 4;
                            Sound.BackGround(3); // Clear BackSounds

                            Sleep(500); // Win

                            Z = (int)Grid.GridColor;

                            ResetMap(0xFFFFFF,false);Redraw();Sleep(SleepTime);
                            ResetMap(Z,false);       Redraw();Sleep(SleepTime);
                            ResetMap(0xFFFFFF,false);Redraw();Sleep(SleepTime);
                            ResetMap(Z,false);       Redraw();Sleep(SleepTime);
                            ResetMap(0xFFFFFF,false);Redraw();Sleep(SleepTime);

                            Grid.GridColor = (COLORREF)Z;

                            FillBrushRect(hdc,0,0,SBuff.Width,SBuff.Height,(HBRUSH)GetStockObject(BLACK_BRUSH));
                            Sleep(500);
                            ResetGame(true);
                            ResetCT();
                        } else if (PelletsEaten == 70 || PelletsEaten == 170) { // Appear Fruit
                            Fruit = true;
                            if (Level > 13) {FruitId = 7;} else {FruitId = FruitTable[Level - 1];}
                        }
                        if (Z == 1) { // Super Pellet Eaten
                            AddScore(40); FrightenedCombo = 0;
                            /*  FrightenedTime Will Range 430 - 230
                             *  In The Span Of 10 Levels
                             *  Keys   (Level 12) Should Be At 230 'Delta Of 200
                             *  Cherry (Level 0 ) Should Be At 430
                             */
                            FrightenedTime = max(430 - ((200 * Level) / 12),230);

                            if (!Blinky.Eaten && !Blinky.IsOnGhostHouse) {Blinky.Frightened = true;}
                            if (!Inky.Eaten && !Inky.IsOnGhostHouse) {Inky.Frightened = true;}
                            if (!Pinky.Eaten && !Pinky.IsOnGhostHouse) {Pinky.Frightened = true;}
                            if (!Clyde.Eaten && !Clyde.IsOnGhostHouse) {Clyde.Frightened = true;}
                        }
                    }


                    Z = Pacman.FacingDirection;
                    if (Keys.Left != Keys.Right) {if (Keys.Left) {Z = 3;} else {Z = 1;}}
                    if (Keys.Up != Keys.Down) {if (Keys.Up) {Z = 0;} else {Z = 2;}}

                    if (Grid.GetBlock(Pacman.X / 8 + FDX[Z],Pacman.Y / 8 + FDY[Z]) > 2) { // Hit a wall
                        Z = Pacman.FacingDirection; // Face Previous Direction, See if That Helps
                        if (Grid.GetBlock(Pacman.X / 8 + FDX[Z],Pacman.Y / 8 + FDY[Z]) > 2) {// Still have Hit A Wall
                        Pacman.Moving = false;//std::cout << "Stopped Moving Wall" << std::endl;
                        } else { // No Longer Hitting a Wall, Lets Keep it that way
                        Pacman.FacingDirection = Z;
                        }
                    } else if (Pacman.FacingDirection != Z) {
                        Pacman.FacingDirection = Z;
                        Pacman.X += FDX[Z];
                        Pacman.Y += FDY[Z];
                    }

                } else { // Check For Back Turning
                    switch (Pacman.FacingDirection) {
                    case 0:{ if (Keys.Down) {Pacman.FacingDirection = 2;} break;}
                    case 1:{ if (Keys.Right) {Pacman.FacingDirection = 1;} break;}
                    case 2:{ if (Keys.Up) {Pacman.FacingDirection = 0;} break;}
                    case 3:{ if (Keys.Left) {Pacman.FacingDirection = 3;} break;}}
                }
                if (Pacman.Moving) {
                    Pacman.X += FDX[Pacman.FacingDirection];
                    Pacman.Y += FDY[Pacman.FacingDirection];
                    if (Pacman.X >= 228) {Pacman.X -= 224;}
                    if (Pacman.X < 4) {Pacman.X += 224;}
                }
            } else {
                Z = 5;
                if (Keys.Left != Keys.Right) {if (Keys.Left) {Z = 3;} else {Z = 1;}}
                if (Keys.Up != Keys.Down) {if (Keys.Up) {Z = 0;} else {Z = 2;}}
                if (Z < 5) {
                    Pacman.FacingDirection = Z;
                    if (Grid.GetBlock(Pacman.X / 8 + FDX[Z],Pacman.Y / 8 + FDY[Z]) <= 2) {Pacman.Moving = true;}
                }
            // Check if it can start moving towards desired direction
            }
        } else if (GameTick % 8 == 0) {
            Pacman.AnimationFrame++; if (Pacman.AnimationFrame >= 20) {
                /* End Of Death Animation */
                //FillBrushRect(hdc,0,0,SBuff.Width,SBuff.Height,(HBRUSH)GetStockObject(BLACK_BRUSH));
                //Sleep(100);
                AddLife(false);
                if (Lives >= 0) {ResetGame(false);} else {
                    Lives = -1; // Game Over //
                    DrawString("GAME  OVER",10,112,160,0xFF,1);
                    Pause = false;
                    if (HiScore > OldHiScore) {SetStoredHiScore(HiScore);OldHiScore = HiScore;}
                    do {
                        StretchBlt(hdc,0,0,SBuff.Width,SBuff.Height,GBuff.Hdc,0,0,GBuff.Width,GBuff.Height,SRCCOPY);
                        Sleep(50);
                    } while (!Pause && DoEvents());
                    if (Pause) {
                        FillBrushRect(hdc,0,0,SBuff.Width,SBuff.Height,(HBRUSH)GetStockObject(BLACK_BRUSH));
                        Sleep(250);
                    return true;
                    } else {return false;}

                }
            };
        } /*  END OF TICKING  */

        Redraw();
        GameTick++;
    *EndLoop = false;
    return false;
}

bool MainProc() {
    bool EndLoop;
    bool Result;

    Sound.IsSoundOn = true;

    int X,Y,Z;

    Fruit = false;

    GameTick = 0;
    LastPelletEaten = 0; // Time
    Score = 0;
    PelletsEaten = 0;
    Lives = 3;
    HasDied = false;
    FrightenedTime = 0;
    FrightenedCombo = 0;
    ScatterMode = true;

    Pause = false;

    Grid.GridColor = RGB(27,27,255);
    GBuff.Clear(0);
    Level = 0;
    ResetGame(true);

    DrawString("POINTS",6,112,4,0xFFFFFF,1);
    DrawNumber(0,112,12,0xFFFFFF,1);
    DrawString("HISCORE",7,0,4,0x10101 * 200,0);
    DrawNumber(HiScore,0,12,0x10101 * 200,0);
    Lives++;// Redraw Lives Counter
    AddLife(false);
    Pacman.Moving = true;
    DrawString("READY!",6,112,160,0x00FFFF,1);
    Redraw();
    BackSoundState = 4;

    if (true) {
        Sound.GameStart();
        for (Z = 0; Z < 70;Z++) {
            Sleep(40);
            DoEvents();
        }
    }
    FillBrushRect(GBuff.Hdc,88,160,48,8,(HBRUSH)GetStockObject(BLACK_BRUSH));

    ResetCT();

    do {
        CTime.QuadPart += 16 * Frequency;
        WaitUntil(CTime); DoEvents();

        Result = MainLoop(&EndLoop);
        if (EndLoop || KillMainLoop) {
            if (HiScore > OldHiScore) {SetStoredHiScore(HiScore);OldHiScore = HiScore;}
            return Result && !KillMainLoop;
        }
    } while (true);
}

#include <mutex>
using std::mutex;

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow) {
    char * S = lpszArgument;
    while (*S) {
        if (*S == 'i') {CheatMode = true;break;}
        else if (*S == 0) {break;}
        S++;
    }
    cout << __cplusplus << '\n';
    HWND hwnd;               /* This is the handle for our window */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    GetModuleFileNameA(NULL,&SrcPath[0],260);
    for (int Z = 0;Z < 260;Z++) {
        if (SrcPath[Z] == '\\') {PathEnd = Z;}
    } PathEnd++;

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = 0;// LoadIcon (NULL, "MAINICON");
    wincl.hIconSm = 0;// LoadIcon (NULL, "MAINICON");
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH)GetStockObject(0);
    //wincl.hbrBackground = CreateSolidBrush(0xFFFFFF);

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    DWORD WinStyle = WS_OVERLAPPEDWINDOW - WS_MAXIMIZEBOX - WS_THICKFRAME;
    hwnd = CreateWindowEx (
           WS_EX_DLGMODALFRAME,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           _T("C++ Pacman"),       /* Title Text */
           WinStyle , /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           224,                 /* The programs width */
           288,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );
    RECT R;

    /* Make the window visible on the screen */

    hdc = GetDC(hwnd);

    GetWindowRect(hwnd,&R);
    Zoom = min(GetDeviceCaps(hdc,HORZRES) / 244,GetDeviceCaps(hdc,VERTRES) / 288);
    R.bottom = R.top + 288*Zoom;
    R.right = R.left + 224*Zoom;
    AdjustWindowRect(&R,WinStyle,false);
    MoveWindow(hwnd,R.left,R.top,R.right-R.left,R.bottom-R.top,true);

    // Initialize //
    TempBuff.SetSize(16,16);
    FTempBuff.SetSize(20,7);
    SBuff.SetSize(224*Zoom,288*Zoom); SBuff.Clear(0);
    BBuff.SetSize(224,288); BBuff.Clear(0);
    GBuff.SetSize(224,288); GBuff.Clear(0);
    SetFilePath("assets\\Mono.bmp");
    bool B = TBuff.LoadFromFile(Path,72,128); // 1 Bit //
    B = B && NotTBuff.LoadFromFile(Path,72,128); // 1 Bit //
    SetFilePath("assets\\Text.bmp");
    B = B && TextBuff.LoadFromFile(Path,23,240); // 1 Bit //
    SetFilePath("assets\\Fruits.bmp");
    B = B && FruitsBuff.LoadFromFile(Path,16,128); // 24 Bit //
    BitBlt(NotTBuff.Hdc,0,0,NotTBuff.Width,NotTBuff.Height,NotTBuff.Hdc,0,0,NOTSRCCOPY);

    {
    LARGE_INTEGER FrequencyFetcher;
    QueryPerformanceFrequency(&FrequencyFetcher);
    Frequency = FrequencyFetcher.QuadPart / 1000; // ms, not s
    }

    Grid.DestHdc = GBuff.Hdc;

    ShowWindow (hwnd, nCmdShow);

    OldHiScore = GetStoredHiScore();
    HiScore = OldHiScore;

    do {Pause = false;} while (MainProc());

    ReleaseDC(hwnd,hdc);

    return 0;
}

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    /*  This function is called by the Windows function DispatchMessage()  */
    union {int Int;struct{short X;short Y;};} L;

    switch (message)                  /* handle the messages */
    {
        case WM_DESTROY: {
            Sound.Terminate();
            KillMainLoop = true;
            break;
        }
        case WM_LBUTTONDOWN: {
            //KillMainLoop = true;
            //mciSendString("play \"C:\\PowerUp.wav\"", NULL, 0, NULL);
            break;
        }
        case WM_MBUTTONDBLCLK: {
            //KillMainLoop = true;
            //mciSendString("play \"C:\\PowerUp.wav\"", NULL, 0, NULL);
            break;
        }
        case WM_SIZING : {
            RECT *R = (RECT*)lParam;
            /*R->bottom = 100;
            R->right = 100;*/
            break;
        }
        case WM_KEYDOWN: {
            if ((lParam & 0x20000000) == 0) {
                if (wParam == 13) {
                    Pause = !Pause;
                } else {
                    if (Lives == -1) {break;}
                    switch(wParam) {
                        case 37:{Keys.Left  = true;break;}
                        case 38:{Keys.Up    = true;break;}
                        case 39:{Keys.Right = true;break;}
                        case 40:{Keys.Down  = true;break;}
                    } break;
                }
            }
        }
        case WM_KEYUP: {
            if (Lives == -1) {break;}
            switch(wParam) {
                case 37:{Keys.Left  = false;break;}
                case 38:{Keys.Up    = false;break;}
                case 39:{Keys.Right = false;break;}
                case 40:{Keys.Down  = false;break;}
            } break;
        }
        default:{                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
        }
    }

    return 0;
}
