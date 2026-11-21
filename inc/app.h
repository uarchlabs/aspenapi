// ===================================================================
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Jeff Nye, uarchlabs.com
// SPDX-FileCopyrightText: 2026 Jeff Nye <jeff@uarchlabs.com>
// ===================================================================
#pragma once
#include <QtCore>
#include <QtGui>
#include <map>

namespace app
{
  static const Qt::CheckState CHK = Qt::Checked;
  static const Qt::CheckState UCK = Qt::Unchecked;

  enum ReplacementType
  {
    REPLACE_NULL,
    REPLACE_PLRU,
    REPLACE_MRU,
    REPLACE_FIFO,
    REPLACE_LIFO,
    REPLACE_RANDOM,
    REPLACE_LAST
  };
  typedef ReplacementType ReplacementType;

  enum WriteHitPolicy
  {
    WHP_NULL,
    WHP_WRITE_BACK,
    WHP_WRITE_THRU,
    WHP_LAST
  };
  typedef WriteHitPolicy WriteHitPolicy;

  enum WriteMissPolicy
  {
    WMP_NULL,
    WMP_WRITE_ALLOC,
    WMP_WRITE_AROUND,
    WMP_LAST
  };
  typedef WriteMissPolicy WriteMissPolicy;

  enum WritePropPolicy
  {
    WPP_NULL,
    WPP_WRITE_INVAL,
    WPP_WRITE_COPY,
    WPP_LAST
  };
  typedef WritePropPolicy WritePropPolicy;

  enum IdxTagAttributes
  {
    IDXTAGS_NULL,
    IDXTAGS_PHYS_PHYS,
    IDXTAGS_PHYS_VIRT,
    IDXTAGS_VIRT_PHYS,
    IDXTAGS_VIRT_VIRT,
    IDXTAGS_LAST
  };
  typedef IdxTagAttributes IdxTagAttributes;

  enum MemoryArch
  {
    MEMARCH_NULL,
    MEMARCH_HARVARD,
    MEMARCH_UNIFIED,
    MEMARCH_LAST
  };
  typedef MemoryArch MemoryArch;

  enum OverlayType
  {
    OVRTYPE_NULL,
    OVRTYPE_SMP,
    OVRTYPE_SHARED,
    OVRTYPE_DIST,
    OVRTYPE_XIMD,
    OVRTYPE_SYSTOLIC,
    OVRTYPE_GPU,
    OVRTYPE_STREAM,
    OVRTYPE_CUSTOM,
    OVRTYPE_LAST
  };
  typedef OverlayType OverlayType;

  enum CoherencyProtocol
  {
    PROTOCOL_NULL,
    PROTOCOL_AUTO,
    PROTOCOL_MESI,
    PROTOCOL_MOESI,
    PROTOCOL_MSI,
    PROTOCOL_MOSI,
    PROTOCOL_MERSI,
    PROTOCOL_MESIF,
    PROTOCOL_LAST
  };
  typedef CoherencyProtocol CoherencyProtocol;

  enum FileType {
    FILETYPE_NONE,
    FILETYPE_MDL,
    FILETYPE_AB_SESSION,
    FILETYPE_LOG,
    FILETYPE_LAST
  };
  typedef FileType FileType;

  enum SceneZValues
  {
    shadowZ           = 0,
    connectionZ       = 1,
    interfaceZ        = 2,
    funcunitZ         = 3,
    LAST_SceneZValues = 4
  };

  enum KeyCodeModifiers
  {
    key_Mod_None=0,
    key_Mod_Shft=1,
    key_Mod_Ctrl=4,
    key_Mod_Alt=8
  };
  typedef KeyCodeModifiers KeyCodeModifiers;

  enum KeyCode
  {
    // T plain text  V Virtual key  S Scan code  M native modifiers
    key_Esc= 65307,  //T ''  V 65307 S   9 M 0
    key_F1 = 65470,  //T ''  V 65470 S  67 M 0
    key_F2 = 65471,  //T ''  V 65471 S  68 M 0
    key_F3 = 65472,  //T ''  V 65472 S  69 M 0
    key_F4 = 65473,  //T ''  V 65473 S  70 M 0
    key_F5 = 65474,  //T ''  V 65474 S  71 M 0
    key_F6 = 65475,  //T ''  V 65475 S  72 M 0
    key_F7 = 65476,  //T ''  V 65476 S  73 M 0
    key_F8 = 65477,  //T ''  V 65477 S  74 M 0
    key_F9 = 65478,  //T ''  V 65478 S  75 M 0
    key_F10= 65479,  //T ''  V 65479 S  76 M 0
    key_F11= 65480,  //T ''  V 65480 S  95 M 0
    key_F12= 65481,  //T ''  V 65481 S  96 M 0

    //PAN
    key_Left=65361,         //T '' V 65361 S 100 M 0
    key_Up=65362,           //T '' V 65362 S 98 M 0
    key_Right=65363,        //T '' V 65363 S 102 M 0
    key_Down=65364,         //T '' V 65364 S 104 M 0

    key_Pad_Left=65430,     //T '' V 65430 S 83 M 0
    key_Pad_Up=65431,       //T '' V 65431 S 80 M 0
    key_Pad_Right=65432,    //T '' V 65432 S 85 M 0
    key_Pad_Down=65433,     //T '' V 65433 S 88 M 0

    key_Pad_Home=65429,     //T ''  V 65429 S  79 M 0
    key_Pad_PageUp=65434,   //T ''  V 65434 S  81 M 0
    key_Pad_PageDown=65435, //T ''  V 65435 S  89 M 0
    key_Pad_End=65436,      //T ''  V 65436 S  87 M 0

    key_Home=65360,         //T ''  V 65360 S  97 M 0
    key_PageUp=65365,       //T ''  V 65365 S  99 M 0
    key_PageDown=65366,     //T ''  V 65366 S 105 M 0
    key_End=65367,          //T ''  V 65367 S 103 M 0

    key_o=111,              //T 'o' V   111 S  32 M 0

    //ZOOM
    key_Plus=43,            //T '+' V    43 S  21 M 0
    key_Minus=45,           //T '-' V    45 S  20 M 0
    key_Pad_Plus=65451,     //T '+' V 65451 S  86 M 0
    key_Pad_Minus=65453,    //T '-' V 65453 S  82 M 0
    key_Z=90,               //T 'Z' V    90 S  52 M 0
    key_z=122,              //T 'z' V   122 S  52 M 0
    key_r=114,              //T ''  V   114 S  27 M 0

    //MODS
    key_g=103,              //T ''  V   103 S  42 M 0
    key_G=71,               //T ''  V    71 S  42 M 1
    key_Shft=65505,         //T ''  V 65505 S  50 M 0
    key_LeftCtl=65507,      //T ''  V 65507 S  37 M 0
    key_RightCtl=65508,     //T ''  V 65508 S 109 M 0
    key_LeftAlt=65513,      //T ''  V 65513 S  64 M 0
    key_RightAlt=65514,     //T ''  V 65514 S 113 M 0

    key_Ins=65579,          //T ''  V 65379 S 106 M 0
    key_Del=65535,          //T ''  V 65535 S 107 M 0

    key_Pad_Ins=65438,      //T ''  V 65438 S  90 M 0
    key_Pad_Del=65439,      //T ''  V 65439 S  91 M 0

    //EDIT w/ ctrl modifier
    key_c=99,               //T ''  V    99 S  54 M 0
    key_x=99,               //T ''  V    99 S  54 M 0
    key_v=118,              //T ''  V   118 S  55 M 0
    key_s=115,              //T 's' V   115 S  39 M 0
    key_u=117,              //T 'u' V   117 S  30 M 0
 
    //Flight lines
    key_R=82,               //T 'R' V    82 S  27 M 1
    key_f=102,              //T 'f' V   102 S  41 M 0
    key_F=70,               //T 'F' V    70 S  41 M 0
    //'R'

    //APP - wont be used will use Ctrl-Q, the Qt typical mechanism
    key_q=113,              //T 'q' V   113 S  24 M 0
    key_Q=81                //T 'Q' V    81 S  24 M 0
  }; 
  typedef KeyCode KeyCode;

  typedef struct sSessionInfo
  {
    QString version;
    QString sessionFileName;
    QString designer;
    QString company;
    QString designName;
    int viewScaleIndex;
    QPointF mainWindowOrigin;
    QPointF viewOrigin;
    QString geometry;
  } SessionInfo;

  typedef QMap<QString,QColor> SessionColorMap;
  typedef QMap<int,QColor>     SessionIntColorMap;

  typedef struct sRealPair   { qreal hi;   qreal lo;   } RPair;
  typedef struct sStringPair { QString hi; QString lo; } SPair;
  typedef std::list<qreal> RList;
  typedef struct sNameRealPair { QString name; qreal value; } NameRealPair;
};
