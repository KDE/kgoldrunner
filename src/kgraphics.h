/***************************************************************************
                         kgrgraphics.h  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    copyright            : (C) 2002 by Marco Krüger and Ian Wadham
    email                : See menu "Help, About KGoldrunner"
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KGRAPHICS_H
#define KGRAPHICS_H

// List of colour schemes.
static const char * colourScheme [] = {
    "KGoldrunner",
    "#5a5a9b",     /* Border - periwinkle blue */
    "#ffffff",     /* Title-text - pure-white. */
    ". c #c8b0a0", /* Background */
    "; c #b8a090", /* Background mortar */
    "o c #ff0000", /* Solid light */
    "x c #c05040", /* Solid */
    "s c #b00020", /* Solid dark */
    "+ c #500000", /* Mortar */
    ": c #b47444", /* Ladder light */
    "# c #845424", /* Ladder dark */
    "a c #ffffff", /* Pole or bar */
    "Apple II",
    "#8a8acb",     /* Border - Apple II blue */
    "#ffffff",     /* Title-text - pure-white. */
    ". c #001020", /* Background */
    "; c #001020", /* Background mortar */
    "o c #8a8acb", /* Solid light */
    "x c #8a8acb", /* Solid */
    "s c #8a8acb", /* Solid dark */
    "+ c #001020", /* Mortar */
    ": c #dddddd", /* Ladder light */
    "# c #dddddd", /* Ladder dark */
    "a c #dddddd", /* Pole or bar */
    "Ice Cave",
    "#aabaf0",     /* Border - pale blue */
    "#ffffff",     /* Title-text - pure-white. */
    ". c #efefff", /* Background */
    "; c #d0dfef", /* Background mortar */
    "o c #ffffff", /* Solid light */
    "x c #d0f0ff", /* Solid */
    "s c #b0d8f0", /* Solid dark */
    "+ c #a8c8ff", /* Mortar */
    ": c #ffffff", /* Ladder light */
    "# c #f9d26a", /* Ladder dark */
    "a c #40a0ff", /* Pole or bar */
    "Midnight",
    "#5a5a9b",     /* Border - periwinkle blue */
    "#ffffff",     /* Title-text - pure-white. */
    ". c #000040", /* Background */
    "; c #000020", /* Background mortar */
    "o c #880000", /* Solid light */
    "x c #702820", /* Solid */
    "s c #680010", /* Solid dark */
    "+ c #200000", /* Mortar */
    ": c #563622", /* Ladder light */
    "# c #422a12", /* Ladder dark */
    "a c #666666", /* Pole or bar */
    "KDE Kool",
    "#aabaf0",     /* Border - pale blue */
    "#ffffff",     /* Title-text - pure-white. */
    ". c #eef7ff", /* Background */
    "; c #eef7ff", /* Background mortar */
    "o c #ecfdfe", /* Solid light */
    "x c #c1dafe", /* Solid */
    "s c #c1dafe", /* Solid dark */
    "+ c #9a9afe", /* Mortar */
    ": c #f9d26a", /* Ladder light */
    "# c #c19a68", /* Ladder dark */
    "a c #af7516", /* Pole or bar */
    ""             /* TERMINATOR */
};

/* XPM - Background brick or square (free space) */
static const char * hgbrick_xpm []={
"16 16 9 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
"...........;....",
"...........;....",
"...........;....",
"...........;....",
"...........;....",
"...........;....",
"...........;....",
";;;;;;;;;;;;;;;;",
"....;...........",
"....;...........",
"....;...........",
"....;...........",
"....;...........",
"....;...........",
"....;...........",
";;;;;;;;;;;;;;;;"};

/* XPM - Editor's icon for the hero */
static const char * edithero_xpm []={
"16 16 13 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
"c c #008000",
"a c #00c000",
"b c #00ff00",
"d c #808080",
"...........;....",
"...........;....",
"..........a;....",
"........baacc...",
"........bcccc...",
".......bccca....",
"....caacccccc...",
";;;;bccacccaccc.",
"...;bccaacccaccc",
"...;caacccc.....",
"...;..dbccca....",
"...;.ccacccca...",
"...bcccccaccc...",
"...ac....bccc...",
"...;.....bccc...",
";;;;;;;;;caccc;;"};

/* XPM - Editor's icon for an enemy */
static const char * editenemy_xpm []={
"16 16 13 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
"d c #000080",
"c c #0000ff",
"b c #008080",
"a c #00ffff",
"...........;....",
"...........;....",
"...........;....",
"........abbcc...",
"........acccc...",
".......acccd....",
"....bbbcccccc...",
";;;;accbcccbccbd",
"....accbbccbbccc",
"....bbbcccc.bdd.",
"....;..acccb....",
"....;bbbccccc...",
"...acccccbccc...",
"...bcddddaccc...",
"....;....accc...",
";;;;;;;;;bbccc;;"};

/* XPM - Ladder */
static const char * ladder_xpm []={
"16 16 9 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
":##........;.:##",
":##........;.:##",
":##........;.:##",
":#::::::::::::##",
":############:##",
":##........;.:##",
":##........;.:##",
":##;;;;;;;;;;:##",
":##.;........:##",
":##.;........:##",
":##.;........:##",
":#::::::::::::##",
":############:##",
":##.;........:##",
":##.;........:##",
":##;;;;;;;;;;:##"};

/* XPM - Hidden ladder (for Editor only) */
static const char * hladder_xpm []={
"16 16 9 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
":##........;.:##",
":##........;.:##",
":##........;.:##",
":#:::::::..;.:##",
":########..;.:##",
":##........;.:##",
":##........;.:##",
":##;;;;;;;;;;:##",
":##.;........:##",
":##.;........:##",
":##.;........:##",
":##.;..:::::::##",
":##.;..######:##",
":##.;........:##",
":##.;........:##",
":##;;;;;;;;;;:##"};

/* XPM - Nugget */
static const char * nugget_xpm []={
"16 16 12 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
"a c #c0b000",
"c c #e08000",
"b c #ffff00",
"...........;....",
"...........;....",
"...........;....",
"...........;....",
"...........;....",
"...........;....",
"......bba..;....",
";;;;bbbbbcca;;;;",
"...bbbababacc...",
"..abbbababaac...",
"..bbbbbababaa...",
"..bbbbbbabaac...",
"..abbababacca...",
"...ababacacc....",
"....acacacc;....",
";;;;;;aaa;;;;;;;"};

/* XPM - Pole or bar */
static const char * pole_xpm []={
"16 16 9 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
"...........;....",
"...........;....",
"aaaaaaaaaaaaaaaa",
"...........;....",
"...........;....",
"...........;....",
"...........;....",
";;;;;;;;;;;;;;;;",
"....;...........",
"....;...........",
"....;...........",
"....;...........",
"....;...........",
"....;...........",
"....;...........",
";;;;;;;;;;;;;;;;"};

/* XPM - Concrete */
static const char * beton_xpm []={
"16 16 9 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
"sxssxssxssxsssss",
"sxxsxxsxxsxsxxsx",
"xsxsxsxssxxssxxs",
"xssxsxxsxssxsxss",
"sxxsxssxsxsxxsxs",
"xxsxxxssxsxsxxsx",
"sxsxssxxsxxssxsx",
"xsxsxssxsxsxsxss",
"xssxxsxssxssxsxs",
"xsxsxssxssxxxsxx",
"sxxssxxsxxsxsxss",
"sxsxsssxsxsxsxxs",
"sxssxxssxsxssxss",
"ssxssxsxsxsxsxxs",
"xsxxsxssxsxxsxss",
"++++++++++++++++"};

/* XPM - Bricks (10 pics - from whole brick to background [hole] and back) */
static const char * bricks_xpm []={
"160 16 9 1",
". c #c8b0a0", /* Background */
"; c #b8a090", /* Background mortar */
"o c #ff0000", /* Solid light */
"x c #c05040", /* Solid */
"s c #b00020", /* Solid dark */
"+ c #500000", /* Mortar */
": c #b47444", /* Ladder light */
"# c #845424", /* Ladder dark */
"a c #ffffff", /* Pole or bar */
"sosossosos++osssssx;x;.x;;++s++sx;+;x;.x;;;;;+++x;x+x+.x;;;;;;.;;;;;;;.;;;;;;+.;...........;....osos+;.x+++;ssossososossos++osssososososso++ossssossossoso++osss",
"sxsxsxsxsx++oxsxsx+..;...x;++;+x+....;...++;;+++.....;+.+x;;;..;.....;...x;;+..;...........;....xs;++;...x;;+++xsx+;;+;+;+++o+xxxsxsx;sxsx++oxsx+xsxsxxxsx++osxs",
"xsxsxsxxsx++osxsxxsxs;+x;.;+osxs+;+;++;x;.;;x;+;+x;.x.;x;.;;++++;x;.x.;x;.;;x;.;...........;....+;+;x+;x;.+;+;sxs+;++++++;++o++xsxxx+++sxx++osxssxsxssxsxx++oxsx",
"xsxssxsxsx++oxxsxxssxsx+x+++osxsxsxs;..+;x;;++;s+++.;....x+;;.;+..;.;.+..x;;;.;;...........;....s++.;....x+;;+;xsxs++.+++++;s++xsssx++xsx++;osxxxsxxxxsxss++oxss",
"xsxxxsxxsx++osxssxsxsxsxsx++osxss+++x+++++;;x++;+.x.x;.+.;+;x+x;;.x.x;.;.;;;x.x;...........;....;.x.++.;.;;;++x.s;+xx++;.;+;o;+sxxxsx+.;.+;;s+sxsxssxsxx+x++osxs",
"xsxsx+sxss++oxsxxxxxsxsxss++osxxsx+++;+;+;+;++sxs;.+..x.;x;;.+++.;.;..x.++;;.;.x...........;....+;...+x.;x;;.+.xxs+;.+x.+;+;o+;ssxs++.x.+;+;osxxsxx++;s+++++oxsx",
"ssxxsxxxsx++oxssxssxxsxxxs++oxsxxs+;+++++++;oxss;+;.;;;;++;;++;s;x+.;;;+.;;;;.;+...........;....;x....+..;;;+.;;s;++;;+;.+;;s++xsxxs;;;;.+;;oxsxsx+sxsx;.;++oxsx",
"+++++++++++++++++++++++++++++++++++++++;+++;++++;;;;;+;;;;;;++++;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;+;;;;;;;;;;++;;++;;;;;;;;;;;++++++++;;;;;;;++++++++++;;;;;+++++",
"sss++ossssossssssss++ossssosssososo++osssssossso+;;+;s;+;x++++so;...;x;++x;x+.x+....;...........x....x+x;x;x;.x;s+++;x;x;x;x;++ssss++++x;x++++++sso++osx;xs+ssso",
"xsx++osxsx+xsxxsxsx++oxsxxsxsxsxxsx++oxsxsxsxxsxsxx+;+++...+;+;s++x+;;......;+......;.............x+;;......+...x+;+;+......++;sxsx++s;..+;+;+;sxsx++ox.+s++xsxs",
"sxs++oxsxsxsxssx+sx++osxs+xsxxx+sxs++osx+xs+ss;sxs;+;++;++.;+++x.;;;;x++;++x+;+;....;...........;+;;;x..x..x;.++s+++;+;;;;;s;+xsxxs+++++;+sx++xsxxs++osxx+sxsxxx",
"xsx++oxssxsxsxssxss++oxsxsxxsssxsxx++oxsssxxsxssxsx++os;+xs+xsxsx.++;+++++++++++....;...........++x+;.....;.+.;+x+;++os..s;+++++xsx++ox++;s+xxsxxsx++oxs++s++xss",
"xsx++oxsxxxsxxxsxxx++oxsxsx+xsxxxss++osxxxssxsxxsxx++oxssxsxsxxs;+;;;;+;+;++;+x+....;...........;+;;;+.x;.+;+..+s;+++ox+;;++;+++sxx++os;++;xssxssxx++osxsxsxssxx",
"xsx++ox+xssxssxsxsx++oxxssxsxxssxxx++osxxxxsxxsxssx++osxsxxsxssxsx;;;+++++x+++++....;...........xs++;.+.++;s+.+;sxs++o++sx+x++;;ssx++oxsxs+sxsxxsxs++oxsxsxxxxsx",
"sxs++osssxxsxxsxxsx++osxsxsxssxssxs++oxssxsxsxsxxxs++osxxsxsxsxssx+;;+;;;+++++++....;...........s;s+;.x..x.;+;sxx+x++o+;++++++sxsxs++ossxs;sxsxsxxs++oxssxssxssx",
"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++;;;;;;++;+++;;;;;;;;;;;;;;;;+;++;;;;;;;;;+++++++++++++++++++++++++++++++++++++++++++++++++++"};
#endif // KGRAPHICS_H
