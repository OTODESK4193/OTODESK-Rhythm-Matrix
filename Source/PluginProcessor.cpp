// ==============================================================================
// Source/PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

static const std::array<InstrumentPatch, PATCH_MAX> patchLibrary = { {
        // wave, freq, pDec, pAmt, aAtt, aDec, noise, fTyp, fFreq, fRes, drive, vol
        /* K_909 */      {0, 48.0f,  10.0f,  5.0f, 1.0f, 40.0f,  0.05f, 0, 2000.0f, 1.0f, 1.5f, 1.1f},
        /* K_808 */      {0, 42.0f,  15.0f,  3.0f, 1.0f, 80.0f,  0.0f,  0, 1000.0f, 1.0f, 1.8f, 1.2f},
        /* K_Acoustic */ {1, 55.0f,  10.0f,  2.0f, 1.0f, 30.0f,  0.2f,  0, 1500.0f, 1.5f, 1.2f, 1.0f},
        /* K_Deep */     {0, 35.0f,  10.0f,  1.5f, 2.0f, 50.0f,  0.0f,  0, 800.0f,  1.0f, 1.2f, 1.1f},
        /* K_Punch */    {2, 60.0f,   8.0f,  5.0f, 1.0f, 35.0f,  0.1f,  0, 3000.0f, 2.0f, 2.5f, 1.0f},
        /* K_Hard */     {3, 60.0f,  10.0f,  6.0f, 1.0f, 40.0f,  0.3f,  0, 4000.0f, 1.5f, 3.0f, 0.9f},
        /* K_Soft */     {0, 50.0f,  10.0f,  1.0f, 2.0f, 40.0f,  0.0f,  0, 500.0f,  1.0f, 1.0f, 1.0f},
        /* K_Sub */      {0, 30.0f,   5.0f,  1.0f, 3.0f, 80.0f,  0.0f,  0, 300.0f,  1.0f, 1.5f, 1.2f},
        /* K_Click */    {0, 80.0f,   5.0f, 10.0f, 0.5f, 15.0f,  0.4f,  0, 5000.0f, 1.0f, 1.0f, 1.0f},
        /* K_FM */       {2, 45.0f,  15.0f,  4.0f, 1.0f, 50.0f,  0.0f,  0, 1200.0f, 2.5f, 2.0f, 1.0f},

        /* S_909 */      {1, 180.0f, 10.0f,  1.5f, 1.0f, 40.0f,  0.8f,  1, 400.0f,  1.2f, 1.5f, 0.9f},
        /* S_808 */      {0, 200.0f,  8.0f,  2.0f, 1.0f, 30.0f,  0.7f,  1, 600.0f,  1.0f, 1.2f, 0.9f},
        /* S_Tight */    {0, 250.0f,  5.0f,  1.0f, 1.0f, 20.0f,  0.9f,  1, 800.0f,  1.0f, 1.8f, 0.9f},
        /* S_Fat */      {2, 150.0f, 10.0f,  2.0f, 1.0f, 40.0f,  0.6f,  0, 5000.0f, 1.5f, 2.5f, 0.9f},
        /* S_Rim */      {0, 400.0f,  5.0f,  1.5f, 1.0f, 15.0f,  0.1f,  2, 1200.0f, 3.0f, 1.2f, 1.0f},
        /* S_Clap */     {2, 100.0f,  8.0f,  0.0f, 3.0f, 35.0f,  1.0f,  2, 1500.0f, 1.0f, 1.5f, 0.9f},
        /* S_Snap */     {0, 800.0f,  5.0f,  0.0f, 1.0f, 15.0f,  0.8f,  1, 2000.0f, 1.0f, 1.0f, 0.9f},
        /* S_Noise */    {0, 100.0f,  0.0f,  0.0f, 1.0f, 30.0f,  1.0f,  1, 1000.0f, 1.0f, 1.5f, 0.9f},
        /* S_Lofi */     {3, 150.0f, 10.0f,  1.0f, 1.0f, 30.0f,  0.5f,  2, 800.0f,  2.0f, 3.0f, 0.8f},
        /* S_Acoustic */ {1, 220.0f, 10.0f,  1.2f, 1.0f, 35.0f,  0.6f,  0, 6000.0f, 1.0f, 1.2f, 0.9f},

        /* H_Closed */   {3, 800.0f,  5.0f,  0.0f, 1.0f, 15.0f,  1.0f,  1, 4000.0f, 1.0f, 1.0f, 0.63f},
        /* H_Open */     {3, 800.0f,  5.0f,  0.0f, 1.0f, 35.0f,  1.0f,  1, 4000.0f, 1.0f, 1.0f, 0.63f},
        /* H_Fast */     {3, 900.0f,  2.0f,  0.0f, 1.0f, 10.0f,  1.0f,  1, 6000.0f, 1.0f, 1.0f, 0.54f},
        /* H_Shaker */   {0, 200.0f,  0.0f,  0.0f, 8.0f, 20.0f,  1.0f,  2, 3000.0f, 1.5f, 1.0f, 0.54f},
        /* H_Tambourine */{3,600.0f,  0.0f,  0.0f, 3.0f, 25.0f,  0.8f,  1, 5000.0f, 2.0f, 1.5f, 0.54f},
        /* H_Ride */     {1, 400.0f,  5.0f,  0.0f, 1.0f, 45.0f,  0.6f,  1, 3000.0f, 1.0f, 1.0f, 0.63f},
        /* H_Crash */    {2, 300.0f, 10.0f,  0.0f, 1.0f, 60.0f,  0.9f,  1, 2000.0f, 1.0f, 1.5f, 0.63f},
        /* H_Metallic */ {3, 1200.0f, 5.0f,  0.0f, 1.0f, 25.0f,  0.4f,  2, 4500.0f, 4.0f, 1.2f, 0.54f},

        /* P_TomL */     {0, 80.0f,  15.0f,  2.0f, 1.0f, 40.0f,  0.0f,  0, 1000.0f, 1.0f, 1.5f, 1.1f},
        /* P_TomM */     {0, 120.0f, 10.0f,  1.5f, 1.0f, 35.0f,  0.0f,  0, 1500.0f, 1.0f, 1.5f, 1.0f},
        /* P_TomH */     {0, 180.0f, 10.0f,  1.2f, 1.0f, 30.0f,  0.0f,  0, 2000.0f, 1.0f, 1.5f, 1.0f},
        /* P_Conga */    {0, 250.0f,  8.0f,  1.5f, 1.0f, 30.0f,  0.0f,  2, 500.0f,  2.0f, 1.2f, 1.0f},
        /* P_Bongo */    {0, 400.0f,  8.0f,  1.2f, 1.0f, 25.0f,  0.0f,  2, 800.0f,  2.0f, 1.2f, 1.0f},
        /* P_TablaL */   {0, 120.0f, 15.0f,  3.0f, 1.0f, 35.0f,  0.0f,  2, 300.0f,  3.0f, 1.5f, 1.1f},
        /* P_TablaH */   {0, 280.0f, 10.0f,  2.0f, 1.0f, 30.0f,  0.1f,  2, 600.0f,  2.5f, 1.2f, 1.0f},
        /* P_Wood */     {3, 600.0f,  5.0f,  0.5f, 1.0f, 15.0f,  0.1f,  2, 1000.0f, 3.0f, 1.0f, 0.9f},
        /* P_Cowbell */  {3, 540.0f,  8.0f,  0.2f, 1.0f, 25.0f,  0.0f,  2, 800.0f,  4.0f, 1.5f, 0.81f},
        /* P_Gong */     {2, 100.0f, 20.0f,  0.5f, 1.0f, 70.0f,  0.2f,  2, 400.0f,  5.0f, 2.0f, 0.99f},
        /* P_Clave */    {0, 2000.0f, 2.0f,  0.0f, 1.0f, 10.0f,  0.0f,  3, 0.0f,    0.0f, 1.0f, 0.9f},
        /* P_LogDrum */  {0, 65.0f,  10.0f,  1.0f, 1.0f, 40.0f,  0.05f, 0, 400.0f,  1.5f, 2.0f, 1.2f},

        /* F_Noise */    {0, 100.0f,  0.0f,  0.0f, 2.0f, 30.0f,  1.0f,  2, 2000.0f, 1.0f, 1.0f, 0.8f},
        /* F_SubDrop */  {0, 60.0f,  40.0f,  2.0f, 1.0f, 60.0f,  0.0f,  0, 200.0f,  1.0f, 1.5f, 1.2f},
        /* F_Chaos */    {2, 500.0f, 15.0f, -0.8f, 1.0f, 25.0f,  1.0f,  2, 1000.0f, 8.0f, 3.0f, 0.8f},
        /* F_Laser */    {2, 1500.0f,20.0f,  2.0f, 1.0f, 30.0f,  0.0f,  0, 2000.0f, 2.0f, 1.5f, 0.9f},
        /* F_Wobble */   {1, 40.0f,  20.0f,  0.5f, 5.0f, 60.0f,  0.2f,  0, 800.0f,  4.0f, 3.0f, 1.1f},
        /* F_Pluck */    {2, 440.0f, 10.0f,  0.0f, 1.0f, 25.0f,  0.0f,  0, 1500.0f, 2.0f, 1.0f, 0.9f},
        /* F_Bell */     {0, 880.0f, 20.0f,  0.0f, 1.0f, 45.0f,  0.0f,  3, 0.0f,    0.0f, 1.0f, 0.72f},
        /* F_Marimba */  {0, 300.0f, 15.0f,  0.0f, 1.0f, 35.0f,  0.05f, 0, 1000.0f, 1.0f, 1.0f, 1.0f},
        /* F_Chant */    {3, 200.0f, 10.0f,  0.5f, 5.0f, 30.0f,  0.3f,  2, 1000.0f, 4.0f, 1.5f, 0.9f},
        /* F_Sweep */    {2, 100.0f, 50.0f,  2.0f, 5.0f, 60.0f,  0.8f,  2, 2000.0f, 2.0f, 1.5f, 0.9f},

        {2, 130.8f, 15.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0, 1200.0f, 2.0f, 1.2f, 1.0f},
        {2, 146.8f, 15.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0, 1300.0f, 2.0f, 1.2f, 1.0f},
        {2, 174.6f, 15.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0, 1500.0f, 2.0f, 1.2f, 1.0f},
        {2, 196.0f, 15.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0, 1700.0f, 2.0f, 1.2f, 1.0f},
        {2, 220.0f, 15.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0, 1900.0f, 2.0f, 1.2f, 1.0f},
        {2, 261.6f, 15.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0, 2200.0f, 2.0f, 1.2f, 1.0f},
        {2, 293.6f, 15.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0, 2500.0f, 2.0f, 1.2f, 1.0f},
        {2, 349.2f, 15.0f, 0.0f, 1.0f, 50.0f, 0.0f, 0, 3000.0f, 2.0f, 1.2f, 1.0f},

        // ★ M_ArpPluck
        {1, 440.0f, 5.0f, 0.0f, 0.5f, 20.0f, 0.0f, 0, 2500.0f, 1.5f, 1.0f, 1.0f}
    } };

static const std::array<GenreDefinition, 24> genreTable = { {
    { 4, 4, 125, 135, {"909 Kick", "909 Snare", "CHH", "OHH", "Clap", "Ride", "Tom", "Noise FX"},
      {K_909, S_909, H_Closed, H_Open, S_Clap, H_Ride, P_TomL, F_Noise},
      {{1,2,4,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,5,7,0}, {2,4,8,0}},
      {0,0,0,0,0,0,0,0}, {0,0,2,2,0,5,5,10} },
    { 4, 4, 120, 126, {"Deep Kick", "Rimshot", "Shuff Hat", "Open Hat", "Clap", "Conga", "Bongo", "Vocal Chop"},
      {K_Deep, S_Rim, H_Closed, H_Open, S_Clap, P_Conga, P_Bongo, F_Chant},
      {{1,2,4,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {4,6,8,0}, {2,3,4,0}},
      {-2,0, 5,0,-2, -5, -5, 0}, {2,5, 15,5, 5, 10, 10, 15} },
    { 4, 4, 130, 138, {"Punch Kick", "Snare/Rim", "Garage Hat", "Ride", "Clap", "Sub Bass", "Perc 1", "Perc 2"},
      {K_Punch, S_Rim, H_Fast, H_Open, S_Clap, K_Sub, P_Wood, H_Shaker},
      {{2,3,4,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {2,4,0,0}, {2,3,4,0}, {3,5,0,0}, {5,7,0,0}},
      {-5,5, 10,5, 0, -10, 0, 0}, {5,15, 25,15, 10, 10, 15, 15} },
    { 4, 4, 165, 175, {"Heavy Kick", "Tight Snr", "Fast Hat", "Ride", "Break Rim", "Break 1", "Break 2", "Sub"},
      {K_Hard, S_Tight, H_Fast, H_Ride, S_Rim, P_TomH, P_TomM, F_SubDrop},
      {{2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {4,8,0,0}, {2,4,0,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}},
      {0,0, -5,-5, 0, -10, -10, 0}, {2,2, 5,5, 5, 10, 10, 5} },
    { 4, 4, 135, 150, {"808 Kick", "808 Snare", "Roll Hat", "Open Hat", "Clap", "Perc", "808 Bass", "FX"},
      {K_808, S_808, H_Fast, H_Open, S_Clap, P_Wood, K_Sub, F_Laser},
      {{2,4,0,0}, {2,4,0,0}, {4,6,8,0}, {2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {2,4,0,0}, {2,4,0,0}},
      {0,0, 0,0, 0, 0, 0, 0}, {0,0, 0,0, 0, 0, 0, 0} },
    { 4, 4, 160, 160, {"Juke Kick", "Snare", "Fast Hat", "Hat 2", "Clap", "Tom", "Vocal 1", "Vocal 2"},
      {K_808, S_909, H_Fast, H_Closed, S_Clap, P_TomM, F_Chant, F_Chaos},
      {{3,4,6,0}, {3,4,6,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}, {3,5,6,0}, {3,4,5,0}, {4,6,7,0}},
      {-5,-5, -5,-5, 0, -10, -5, -5}, {5,5, 5,5, 5, 10, 15, 15} },
    { 4, 4, 140, 180, {"Glitch Kick", "Drill Snr", "Hat 1", "Hat 2", "Noise", "Perc 1", "Perc 2", "Glitch FX"},
      {K_Hard, S_Tight, H_Fast, H_Metallic, F_Noise, P_Wood, P_Cowbell, F_Chaos},
      {{4,5,7,0}, {4,6,8,0}, {5,7,9,0}, {6,8,9,0}, {3,5,7,0}, {5,7,9,0}, {4,6,8,0}, {3,5,7,0}},
      {-10,-10, -15,-15, -20, -20, -20, -20}, {10,10, 15,15, 20, 20, 20, 20} },
    { 4, 4, 140, 150, {"Stomp Kick", "Fat Snare", "Hat", "Ride", "Clap", "Wobble", "Growl", "Sub"},
      {K_Hard, S_Fat, H_Closed, H_Ride, S_Clap, F_Wobble, F_Chaos, K_Sub},
      {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,8,0}, {2,4,0,0}, {1,2,4,0}},
      {0,0, 0,0, 0, 0, 0, 0}, {0,0, 5,5, 0, 10, 5, 0} },
    { 4, 4, 95, 115, {"Acoustic Kick", "Snare", "Shaker 1", "Shaker 2", "Clave", "Conga", "Djembe", "Agogo"},
      {K_Acoustic, S_Rim, H_Shaker, H_Shaker, P_Clave, P_Conga, P_Bongo, P_Cowbell},
      {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {3,4,0,0}, {3,4,5,0}, {3,4,6,0}, {2,3,4,0}},
      {0,0, 3,3, 5, 5, 5, 5}, {5,10, 15,15, 20, 20, 20, 20} },
    { 4, 4, 98, 108, {"Heavy Kick", "Snare", "Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Chant"},
      {K_Deep, S_808, H_Closed, H_Open, S_Clap, P_TomL, P_TomM, F_Chant},
      {{3,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}},
      {-5,0, 0,0, -5, -5, -5, 0}, {0,5, 10,5, 0, 10, 10, 10} },
    { 4, 4, 110, 115, {"Log Drum", "Snare/Rim", "Shaker", "Open Hat", "Clap", "Conga", "Woodblock", "Whistle"},
      {P_LogDrum, S_Rim, H_Shaker, H_Open, S_Clap, P_Conga, P_Wood, F_Pluck},
      {{3,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}},
      {0,0, 5,0, 0, 5, 5, 0}, {10,5, 15,5, 5, 15, 15, 10} },
    { 7, 8, 80, 120, {"Bayan", "Dayan", "Tabla", "Manjira", "Ghungroo", "Dholak 1", "Dholak 2", "Vocal"},
      {P_TablaL, P_TablaH, P_Bongo, H_Open, H_Shaker, P_Conga, P_TomM, F_Chant},
      {{2,3,4,0}, {2,3,4,5}, {2,3,4,5}, {2,3,4,0}, {3,4,5,6}, {2,3,4,0}, {2,3,4,0}, {1,2,3,0}},
      {-5,-5, -5, -2, -2, -5, -5, -10}, {5,5, 5, 5, 5, 5, 5, 10} },
    { 4, 4, 90, 120, {"Surdo", "Caixa", "Pandeiro", "Ganza", "Tamborim", "Agogo", "Cuica", "Repique"},
      {K_Deep, S_Tight, P_Bongo, H_Shaker, P_Wood, P_Cowbell, F_Sweep, P_TomH},
      {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {3,4,0,0}, {3,4,0,0}, {3,4,0,0}, {4,6,0,0}},
      {0,5, 5,5, 5, 5, 5, 5}, {5,15, 20,20, 20, 20, 20, 20} },
    { 4, 4, 90, 105, {"Kick", "Snare (Tresillo)", "Hat", "Open Hat", "Clap", "Timbales", "Perc", "Vocal FX"},
      {K_Punch, S_Tight, H_Closed, H_Open, S_Clap, P_TomH, P_Wood, F_Chant},
      {{2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,0,0}, {2,4,0,0}},
      {0,-6, 0,0, 0, 0, 0, 0}, {5,0, 5,5, 5, 10, 10, 10} },
    { 8, 4, 80, 110, {"Gong", "Kempul", "Kendang", "Bonang", "Saron", "Kenong", "Kethuk", "Slenthem"},
      {P_Gong, P_TomL, P_TablaL, P_Cowbell, F_Marimba, P_TomM, P_Wood, K_Sub},
      {{1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}},
      {-10,-10, -5,-5, -5, -10, -5, -10}, {10,10, 10,10, 10, 10, 10, 10} },
    { 4, 4, 100, 115, {"Kick", "Snare", "Hi-Hat", "Open Hat", "Clap", "Tom", "Conga", "Tambourine"},
      {K_Acoustic, S_Acoustic, H_Closed, H_Open, S_Clap, P_TomM, P_Conga, H_Tambourine},
      {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,6,0}, {4,6,0,0}},
      {0,0, 5,0, 0, 0, 5, 5}, {5,10, 20,5, 5, 10, 15, 20} },
    { 4, 4, 100, 112, {"Punch Kick", "Snare", "Swing Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Orch Hit"},
      {K_Punch, S_Fat, H_Closed, H_Open, S_Clap, P_TomM, P_TomH, F_Pluck},
      {{2,4,0,0}, {2,4,0,0}, {6,8,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,0,0}, {1,2,0,0}},
      {0,0, 15,0, 0, 0, 0, 0}, {5,5, 25,5, 5, 5, 5, 5} },
    { 4, 4, 80, 95, {"Soft Kick", "Rimshot", "Loose Hat", "Ride", "Snap", "Tom", "Shaker", "Vinyl FX"},
      {K_Soft, S_Rim, H_Closed, H_Ride, S_Snap, P_TomL, H_Shaker, S_Lofi},
      {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {3,4,0,0}, {2,4,0,0}, {2,3,4,0}, {4,6,0,0}, {1,2,0,0}},
      {-5, 10, 20, 10, 5, 0, 15, 0}, {2, 25, 40, 25, 20, 10, 30, 0} },
    { 4, 4, 85, 95, {"Gritty Kick", "Fat Snare", "Hi-Hat", "Open Hat", "Clap", "Perc", "Scratch", "Sample"},
      {K_Hard, S_Fat, H_Closed, H_Open, S_Clap, P_Wood, F_Sweep, S_Lofi},
      {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {1,2,4,0}},
      {2, 5, 5, 0, 0, 0, 0, 0}, {8, 12, 15, 5, 5, 10, 5, 0} },
    { 5, 4, 120, 160, {"Kick", "Snare", "Hi-Hat", "Ride", "Ghost Snr", "Tom 1", "Tom 2", "Crash"},
      {K_Acoustic, S_Acoustic, H_Closed, H_Ride, S_Rim, P_TomM, P_TomL, H_Crash},
      {{2,3,4,0}, {2,3,4,0}, {4,5,6,0}, {3,4,5,0}, {4,6,8,0}, {3,4,5,0}, {3,4,5,0}, {1,2,0,0}},
      {0,0, 0,0, 0, 0, 0, 0}, {5,5, 5,5, 5, 5, 5, 5} },
    { 7, 8, 140, 180, {"Heavy Kick", "Fat Snare", "Hi-Hat", "China", "Ghost Snr", "Low Tom", "Mid Tom", "High Tom"},
      {K_Click, S_Fat, H_Closed, H_Metallic, S_Tight, P_TomL, P_TomM, P_TomH},
      {{2,3,4,5}, {2,3,4,0}, {4,6,8,0}, {2,3,4,0}, {4,6,8,0}, {3,4,5,0}, {3,4,5,0}, {3,4,5,0}},
      {0,0, 0,0, 0, 0, 0, 0}, {2,2, 2,2, 5, 5, 5, 5} },
    { 12, 8, 140, 170, {"Clap 1", "Clap 2", "Marimba 1", "Marimba 2", "Woodblock", "Pulse", "Phase 1", "Phase 2"},
      {S_Clap, S_Snap, F_Marimba, F_Marimba, P_Wood, K_Soft, S_Rim, S_Rim},
      {{1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}},
      {0,0, 0,0, 0, 0, -20, 20}, {0,0, 0,0, 0, 0, -20, 20} },
    { 4, 4, 120, 150, {"Node C", "Node D", "Node F", "Node G", "Node A", "Node C^", "Node D^", "Node F^"},
      {M_Pluck1, M_Pluck2, M_Pluck3, M_Pluck4, M_Pluck5, M_Pluck6, M_Pluck7, M_Pluck8},
      {{2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}},
      {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0} },
    { 4, 4, 120, 150, {"Chaos C", "Chaos D", "Chaos F", "Chaos G", "Chaos A", "Chaos C^", "Chaos D^", "Chaos F^"},
      {M_Pluck1, M_Pluck2, M_Pluck3, M_Pluck4, M_Pluck5, M_Pluck6, M_Pluck7, M_Pluck8},
      {{1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}},
      {-20,-20,-20,-20,-20,-20,-20,-20}, {20,20,20,20,20,20,20,20} }
} };

AIDrumMachineAudioProcessor::AIDrumMachineAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
    formatManager.registerBasicFormats();
}

AIDrumMachineAudioProcessor::~AIDrumMachineAudioProcessor() {}

const GenreDefinition& AIDrumMachineAudioProcessor::getGenreDef(int index) {
    if (index < 0 || index >= 24) return genreTable[0];
    return genreTable[index];
}

const InstrumentPatch& AIDrumMachineAudioProcessor::getPatch(PatchID id) {
    if (id < 0 || id >= PATCH_MAX) return patchLibrary[0];
    return patchLibrary[id];
}

juce::String AIDrumMachineAudioProcessor::getNoteName(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 8) return "";
    const char* notesStr[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int note = 60 + arpKey.load() + (trackOctaveUI[trackIndex] * 12) + scalePatterns[arpScale.load()][trackDegreeUI[trackIndex]];
    int n = note % 12;
    int oct = (note / 12) - 1;
    return juce::String(notesStr[n]) + " " + juce::String(oct);
}

void AIDrumMachineAudioProcessor::shiftTrackLeft(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    int totalSteps = trackDivisionsUI[trk] * timeSigNumerator.load() * globalBarCount.load();
    if (totalSteps <= 0) return;
    int firstStep = drumPatternUI[trk][0];
    for (int i = 0; i < totalSteps - 1; ++i) drumPatternUI[trk][i] = drumPatternUI[trk][i + 1];
    drumPatternUI[trk][totalSteps - 1] = firstStep;
    patternUpdated.store(true);
}

void AIDrumMachineAudioProcessor::shiftTrackRight(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    int totalSteps = trackDivisionsUI[trk] * timeSigNumerator.load() * globalBarCount.load();
    if (totalSteps <= 0) return;
    int lastStep = drumPatternUI[trk][totalSteps - 1];
    for (int i = totalSteps - 1; i > 0; --i) drumPatternUI[trk][i] = drumPatternUI[trk][i - 1];
    drumPatternUI[trk][0] = lastStep;
    patternUpdated.store(true);
}

void AIDrumMachineAudioProcessor::clearTrack(int trk) {
    if (trk < 0 || trk >= 8 || trackLocked[trk]) return;
    for (int j = 0; j < 1024; ++j) drumPatternUI[trk][j] = 0;
    patternUpdated.store(true);
}

void AIDrumMachineAudioProcessor::generateAllTracks() {
    int genre = currentGenre.load();
    const auto& def = getGenreDef(genre);
    bool isAlgorithmMode = (genre >= 22);
    bool isSpecialEnsemble = (genre == 14 || genre == 21);

    int fillBar = fillBarTarget.load();
    bool isArp = arpMode.load();
    bool isMono = arpMono.load();
    int curScale = arpScale.load();

    // ★ テンポ・ロックとArpModeでの固定
    if (!tempoLocked.load() && !isSyncEnabled.load() && !isArp) {
        int newBpm = random.nextInt(juce::Range<int>(def.minTempo, def.maxTempo + 1));
        internalTempo.store((double)newBpm);
    }

    int num = timeSigNumerator.load();
    int den = timeSigDenominator.load();

    if (!isArp && genre == 6) {
        const int idmSigs[6][2] = { {4,4}, {5,4}, {5,8}, {7,8}, {7,16}, {15,16} };
        int idx = random.nextInt(6);
        num = idmSigs[idx][0];
        den = idmSigs[idx][1];
        timeSigNumerator.store(num);
        timeSigDenominator.store(den);
    }

    int maxDiv = (den == 16) ? 2 : ((den == 8) ? 4 : 8);
    int bars = globalBarCount.load();

    // ★ Arp Mode 音楽理論・専用ジェネレーター
    if (isArp) {
        int arpPreset = currentGenre.load();
        std::vector<int> usedNotes;

        for (int trk = 0; trk < 8; ++trk) {
            for (int j = 0; j < 1024; ++j) drumPatternUI[trk][j] = 0;
            if (!trackDivLocked[trk]) trackDivisionsUI[trk] = (arpPreset == 7) ? ((trk % 4) + 2) : 4;

            if (!trackLocked[trk]) {
                int degree = 0;
                int octave = 0;
                int attempts = 0;
                bool unique = false;

                // 重複音名の排除とオクターブランダマイズ
                while (!unique && attempts < 50) {
                    switch (arpPreset) {
                    case 0: degree = (trk * 2) % scaleLengths[curScale]; octave = (trk < 4) ? -1 : 0; break;
                    case 1: degree = ((7 - trk) * 3) % scaleLengths[curScale]; octave = (trk < 4) ? 0 : -1; break;
                    case 2: degree = (trk % 3) * 2; octave = (trk / 3) - 1; break;
                    case 3: degree = (trk % 4) * 2; octave = (trk / 4) - 1; break;
                    case 4: degree = (trk * 4) % scaleLengths[curScale]; octave = (trk % 2 == 0) ? -1 : 0; break;
                    case 5: degree = (trk * 5) % scaleLengths[curScale]; octave = (trk % 2 == 0) ? -1 : 1; break;
                    case 6: degree = (trk * 3) % scaleLengths[curScale]; octave = (trk / 3) - 1; break;
                    default: degree = random.nextInt(scaleLengths[curScale]); octave = random.nextInt(3) - 1; break;
                    }

                    if (random.nextInt(100) < 30) degree = (degree + random.nextInt(3)) % scaleLengths[curScale];
                    octave += random.nextInt(juce::Range<int>(-1, 2));
                    octave = juce::jlimit(-2, 2, octave);

                    int noteVal = octave * 12 + degree;
                    if (std::find(usedNotes.begin(), usedNotes.end(), noteVal) == usedNotes.end()) {
                        unique = true;
                        usedNotes.push_back(noteVal);
                    }
                    attempts++;
                }
                trackDegreeUI[trk] = degree;
                trackOctaveUI[trk] = octave;
            }
            else {
                usedNotes.push_back(trackOctaveUI[trk] * 12 + trackDegreeUI[trk]);
            }
        }

        int baseDiv = trackDivisionsUI[0];
        int n = baseDiv * num * bars;
        int currentTrk = 0;
        int currentChordBase = 0;
        bool stepOccupied[1024] = { false };

        for (int j = 0; j < 1024; ++j) {
            if (j >= n) break;

            int currentBarOfStep = j / (baseDiv * num);
            int beatInBar = (j % (baseDiv * num)) / baseDiv;
            bool isFillPortion = (fillBar > 0) && (currentBarOfStep == (fillBar - 1)) && (beatInBar >= num - 2);

            bool stepActive = false;
            // ★ Euclidean Arp 修正 (毎回パターンが変化するように)
            if (arpPreset == 8) {
                int k = (n * (20 + random.nextInt(60))) / 100;
                int eucOffset = random.nextInt(juce::jmax(1, n));
                stepActive = (((j + eucOffset) * k) % n < k);
            }
            else {
                stepActive = (random.nextInt(100) < 70);
            }

            if (isFillPortion) stepActive = (arpPreset % 2 == 0) ? true : false;
            if (!stepActive) continue;

            int vel = 70 + random.nextInt(30);
            std::vector<int> tracksToHit;

            switch (arpPreset) {
            case 0: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 1) % 8; break;
            case 1: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 7) % 8; break;
            case 2: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 2) % 8); tracksToHit.push_back((currentChordBase + 4) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
            case 3: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 2) % 8); tracksToHit.push_back((currentChordBase + 4) % 8); tracksToHit.push_back((currentChordBase + 6) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
            case 4: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 4) % 8; break;
            case 5: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 5) % 8; break;
            case 6: tracksToHit.push_back(currentChordBase % 8); tracksToHit.push_back((currentChordBase + 3) % 8); tracksToHit.push_back((currentChordBase + 6) % 8); currentChordBase = (currentChordBase + 1) % 8; break;
            case 7: break;
            case 8: tracksToHit.push_back(currentTrk); currentTrk = (currentTrk + 1) % 8; break;
            default:
                int nextTrk = random.nextInt(8);
                while (nextTrk == currentTrk) nextTrk = random.nextInt(8);
                currentTrk = nextTrk;
                tracksToHit.push_back(currentTrk);
                break;
            }

            if (arpPreset != 7) {
                if (isMono && tracksToHit.size() > 0) {
                    if (!stepOccupied[j]) { drumPatternUI[tracksToHit[0]][j] = vel; stepOccupied[j] = true; }
                }
                else {
                    for (int t : tracksToHit) drumPatternUI[t][j] = vel;
                }
            }
        }

        if (arpPreset == 7) {
            for (int trk = 0; trk < 8; ++trk) {
                int tDiv = trackDivisionsUI[trk];
                for (int j = 0; j < tDiv * num * bars; ++j) {
                    if (j % tDiv == 0 && random.nextInt(100) < 60) drumPatternUI[trk][j] = 70 + random.nextInt(30);
                }
            }
        }
    }
    else {
        for (int trk = 0; trk < 8; ++trk) {
            if (trackLocked[trk]) continue;

            if (isAlgorithmMode || isSpecialEnsemble) {
                trackCmplxLocked[trk] = false;
                trackComplexity[trk] = (isAlgorithmMode) ? 50 : (10 + random.nextInt(15));
            }

            if (!trackDivLocked[trk]) {
                std::vector<int> candidates;
                for (int i = 0; i < 4; ++i) {
                    if (def.allowedDivs[trk][i] > 0) candidates.push_back(def.allowedDivs[trk][i]);
                }
                int newDiv = 4;
                if (!candidates.empty()) newDiv = candidates[random.nextInt(candidates.size())];

                if (fillBar > 0 && !isAlgorithmMode && !isSpecialEnsemble) {
                    if ((genre == 4 || genre == 7) && trk == 2) newDiv = maxDiv;
                    if ((genre == 0 || genre == 1) && (trk == 1 || trk == 4)) newDiv = maxDiv;
                }

                if (newDiv > maxDiv) newDiv = maxDiv;
                trackDivisionsUI[trk] = newDiv;

                if (!isAlgorithmMode && !isSpecialEnsemble && !trackCmplxLocked[trk]) {
                    trackComplexity[trk] = 20 + random.nextInt(30);
                }
            }

            // ★ エントロピーの変化
            if (!trackEntrpLocked[trk]) {
                trackEntropy[trk] = random.nextInt(juce::Range<int>(10, 50));
            }
            if (!trackShiftLocked[trk]) {
                trackShiftUI[trk] = random.nextInt(juce::Range<int>(def.shiftMin[trk], def.shiftMax[trk] + 1));
            }

            int div = trackDivisionsUI[trk];
            int n = div * num * bars;
            int offset = random.nextInt(juce::Range<int>(0, juce::jmax(1, n)));

            for (int j = 0; j < 1024; ++j) {
                if (j >= n) { drumPatternUI[trk][j] = 0; continue; }

                int currentBarOfStep = j / (div * num);
                int beatInBar = (j % (div * num)) / div;
                int stepInBar = j % (div * num);

                bool isFillActiveBar = (fillBar > 0) && (currentBarOfStep == (fillBar - 1));
                bool isFillPortion = isFillActiveBar && (beatInBar >= num - 2);

                bool isAnchor = false;
                bool isNegativeAnchor = false;
                int anchorVel = 100;
                int cmplx = trackComplexity[trk];
                int entrp = trackEntropy[trk];

                if (isFillPortion && !isAlgorithmMode) {
                    if (genre == 0 || genre == 1) {
                        if (trk == 0) { isNegativeAnchor = true; cmplx = 0; }
                        if (trk == 1 || trk == 4) { isAnchor = true; anchorVel = 50 + (int)(50.0f * ((float)(stepInBar % (div * 2)) / (float)(div * 2))); }
                    }
                    else if (genre == 4 || genre == 7) {
                        if (trk == 2) { isAnchor = true; anchorVel = 80; }
                        else if (trk == 1 && beatInBar == num - 1 && (stepInBar % (div / 2) == 0)) { isAnchor = true; }
                        else if (trk == 0) { isNegativeAnchor = true; cmplx = 0; }
                    }
                    else if (genre == 2 || genre == 5) {
                        if (trk >= 5) { cmplx = 80; }
                        if (trk == 0) { isNegativeAnchor = true; cmplx = 0; }
                    }
                    else if (genre == 19 || genre == 20) {
                        if (trk >= 5) { isAnchor = true; anchorVel = 90; }
                        if (trk == 0) { isAnchor = true; anchorVel = 100; }
                        if (trk == 1 || trk == 2) { isNegativeAnchor = true; cmplx = 0; }
                    }
                    else if (genre == 14 || genre == 21) {
                        if (genre == 14) { isAnchor = (stepInBar % div == 0); cmplx = 0; }
                        else { isNegativeAnchor = true; cmplx = 0; }
                    }
                    else {
                        cmplx = 80;
                        if (trk == 1 || trk == 4) { if (stepInBar % (div / 2) == 0) isAnchor = true; }
                    }
                }
                else if (!isAlgorithmMode) {
                    switch (genre) {
                    case 0: case 1:
                        if (trk == 0 && (stepInBar % div == 0)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                        if (trk == 2 && (stepInBar % div == div / 2)) isAnchor = true;
                        break;
                    case 2:
                        if (trk == 0 && (stepInBar == 0 || stepInBar == div + div / 2 || stepInBar == div * 3 + div / 2)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                        break;
                    case 3:
                        if (trk == 0 && (stepInBar == 0 || stepInBar == div * 2 + div / 2)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                        break;
                    case 4: case 7:
                        if (trk == 0 && (stepInBar == 0 || stepInBar == div + div / 2)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && stepInBar == div * 2) isAnchor = true;
                        break;
                    case 8:
                        if (trk == 0 && (stepInBar == 0 || stepInBar == div + div / 2 || stepInBar == div * 2 || stepInBar == div * 3 + div / 2)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                        break;
                    case 13:
                        if (trk == 0 && (stepInBar % div == 0)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div - 1 || stepInBar == div * 2 + div / 2)) isAnchor = true;
                        break;
                    case 14:
                        if (trk == 0 && stepInBar == 0) isAnchor = true;
                        if (trk == 1 && stepInBar == div * 4 && num > 4) isAnchor = true;
                        if (trk >= 2 && (stepInBar % (div * 2) == 0)) isAnchor = true;
                        break;
                    case 15: case 18:
                        if (trk == 0 && (stepInBar == 0 || stepInBar == div * 2 + div / 2)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3)) isAnchor = true;
                        break;
                    case 19:
                        if (trk == 0 && (stepInBar == 0 || stepInBar == div * 3)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div * 2)) isAnchor = true;
                        break;
                    case 20:
                        if (trk == 0 && (stepInBar == 0 || stepInBar == div * 3 || stepInBar == div * 5)) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div * 3 || stepInBar == div * 5)) isAnchor = true;
                        break;
                    case 21:
                        if (trk == 0 && (stepInBar % (div * 3) == 0)) isAnchor = true;
                        if (trk == 1 && ((stepInBar + div) % (div * 3) == 0)) isAnchor = true;
                        if (trk >= 2 && (stepInBar % 2 == 0)) isAnchor = true;
                        break;
                    default:
                        if (trk == 0 && stepInBar == 0) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3) && num >= 4) isAnchor = true;
                        break;
                    }

                    if (trk == 0 && !isAnchor && !isNegativeAnchor) {
                        if (stepInBar % div == div - 1) {
                            if (random.nextInt(100) < (15 + entrp / 2)) {
                                drumPatternUI[trk][j] = random.nextInt(juce::Range<int>(40, 75));
                                continue;
                            }
                        }
                    }
                }

                int k = juce::jmax(1, (n * cmplx) / 100);
                int vel = 0;

                if (isAnchor) {
                    int velJitter = (int)((entrp / 100.0f) * 20.0f);
                    vel = anchorVel - random.nextInt(juce::Range<int>(0, velJitter + 1));
                }
                else if (isNegativeAnchor) {
                    vel = 0;
                }
                else if (!trackCmplxLocked[trk] && cmplx > 0) {
                    if (genre == 23 && !isSpecialEnsemble) {
                        vel = (random.nextFloat() > (1.0f - cmplx / 100.0f)) ? random.nextInt(juce::Range<int>(40, 101)) : 0;
                    }
                    else {
                        bool isHit = (((j + offset) * k) % n) < k;
                        if (entrp > 0 && random.nextInt(100) < (entrp / 3)) isHit = !isHit;
                        vel = isHit ? random.nextInt(juce::Range<int>(40, 90 + (entrp / 10))) : 0;
                    }
                }
                drumPatternUI[trk][j] = vel;
            }
        }
    }

    patternUpdated.store(true);
    uiNeedsUpdate.store(true);
}

void AIDrumMachineAudioProcessor::loadSample(int trackIndex, const juce::String& filePath) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    juce::File file(filePath);
    if (auto* reader = formatManager.createReaderFor(file)) {
        juce::AudioSampleBuffer tempBuffer(reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&tempBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
        juce::ScopedLock sl(sampleLock);
        sampleBuffers[trackIndex] = tempBuffer;
        hasSample[trackIndex] = true;
        samplePlayPos[trackIndex] = -1;
        delete reader;
    }
}

void AIDrumMachineAudioProcessor::clearSample(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    juce::ScopedLock sl(sampleLock);
    hasSample[trackIndex] = false;
    samplePlayPos[trackIndex] = -1;
}

void AIDrumMachineAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    resetPosition();
    for (int i = 0; i < 8; ++i) {
        synthVoices[i].setSampleRate((float)sampleRate);
        samplePlayPos[i] = -1;
    }

    for (int i = 0; i < 8; ++i) {
        std::memcpy(drumPatternDSP[i], drumPatternUI[i], sizeof(drumPatternUI[i]));
        trackDivisionsDSP[i] = trackDivisionsUI[i];
        trackShiftDSP[i] = trackShiftUI[i];
        trackOctaveDSP[i] = trackOctaveUI[i];
        trackDegreeDSP[i] = trackDegreeUI[i];
    }
    timeSigNumDSP = timeSigNumerator.load();
    timeSigDenDSP = timeSigDenominator.load();
    globalBarCountDSP = globalBarCount.load();
    patternUpdated.store(false);
}

void AIDrumMachineAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AIDrumMachineAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return true;
}
#endif

void AIDrumMachineAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) buffer.clear(i, 0, buffer.getNumSamples());

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    if (patternUpdated.exchange(false)) {
        for (int i = 0; i < 8; ++i) {
            std::memcpy(drumPatternDSP[i], drumPatternUI[i], sizeof(drumPatternUI[i]));
            trackDivisionsDSP[i] = trackDivisionsUI[i];
            trackShiftDSP[i] = trackShiftUI[i];
            trackOctaveDSP[i] = trackOctaveUI[i];
            trackDegreeDSP[i] = trackDegreeUI[i];
        }
        timeSigNumDSP = timeSigNumerator.load();
        timeSigDenDSP = timeSigDenominator.load();
        globalBarCountDSP = globalBarCount.load();
    }

    int numSamples = buffer.getNumSamples();
    float sampleRate = (float)getSampleRate(); if (sampleRate <= 0.0f) sampleRate = 44100.0f;

    double bpm = internalTempo.load();
    bool isPlaying = isPlayingInternal.load();

    if (isSyncEnabled.load()) {
        if (auto* playHead = getPlayHead()) {
            if (auto pos = playHead->getPosition()) {
                if (pos->getBpm().hasValue()) bpm = *pos->getBpm();
                isPlaying = pos->getIsPlaying();
            }
        }
    }
    currentBpm.store(bpm);

    double samplesPerQuarterNote = sampleRate * (60.0 / bpm);
    double samplesPerBeat = samplesPerQuarterNote * (4.0 / (double)timeSigDenDSP);
    int samplesPerBar = (int)(samplesPerBeat * timeSigNumDSP);
    int samplesPerLoop = samplesPerBar * globalBarCountDSP;

    if (samplesPerBar > 0) {
        currentPlayingBar.store((samplesInLoop / samplesPerBar) % globalBarCountDSP);
    }

    bool anySolo = false;
    for (int i = 0; i < 8; ++i) { if (trackSoloed[i]) { anySolo = true; break; } }

    const auto& def = getGenreDef(currentGenre.load());
    bool isArp = arpMode.load();

    juce::ScopedLock sl(sampleLock);

    for (int i = 0; i < numSamples; ++i)
    {
        if (isPlaying && samplesPerLoop > 0) {
            samplesInLoop++;
            if (samplesInLoop >= samplesPerLoop) samplesInLoop = 0;

            for (int trk = 0; trk < 8; ++trk)
            {
                int div = trackDivisionsDSP[trk]; if (div < 1) div = 1;
                int totalStepsInLoop = div * timeSigNumDSP * globalBarCountDSP;
                double samplesPerStep = (double)samplesPerLoop / (double)totalStepsInLoop;
                int shiftInSamples = (int)((trackShiftDSP[trk] / 100.0) * samplesPerStep);

                int virtualSamplesInLoop = samplesInLoop - shiftInSamples;
                while (virtualSamplesInLoop < 0) virtualSamplesInLoop += samplesPerLoop;
                virtualSamplesInLoop %= samplesPerLoop;

                int currentStepForTrack = (virtualSamplesInLoop * totalStepsInLoop) / samplesPerLoop;

                if (currentStepForTrack != trackCurrentStep[trk] && currentStepForTrack < 1024)
                {
                    trackCurrentStep[trk] = currentStepForTrack;
                    bool shouldPlay = true;
                    if (anySolo && !trackSoloed[trk]) shouldPlay = false;
                    if (trackMuted[trk] && !trackSoloed[trk]) shouldPlay = false;

                    int velocity = drumPatternDSP[trk][trackCurrentStep[trk]];
                    if (velocity > 0 && shouldPlay)
                    {
                        if (hasSample[trk]) {
                            samplePlayPos[trk] = 0;
                            sampleVolume[trk] = velocity / 100.0f;
                        }
                        else {
                            InstrumentPatch p = getPatch(isArp ? M_ArpPluck : def.trackPatches[trk]);
                            if (isArp) {
                                int note = 60 + arpKey.load() + (trackOctaveDSP[trk] * 12) + scalePatterns[arpScale.load()][trackDegreeDSP[trk]];
                                p.freq = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
                            }
                            synthVoices[trk].trigger((float)velocity, p);
                        }
                    }
                }
            }
        }

        float mixOut = 0.0f;
        for (int trk = 0; trk < 8; ++trk)
        {
            float osc = 0.0f;
            if (hasSample[trk]) {
                if (samplePlayPos[trk] >= 0 && samplePlayPos[trk] < sampleBuffers[trk].getNumSamples()) {
                    osc = sampleBuffers[trk].getSample(0, samplePlayPos[trk]) * sampleVolume[trk];
                    samplePlayPos[trk]++;
                }
            }
            else {
                osc = synthVoices[trk].process();
            }
            mixOut += osc * 0.5f;
        }

        if (mixOut > 1.0f) mixOut = 1.0f;
        if (mixOut < -1.0f) mixOut = -1.0f;

        if (leftChannel != nullptr) leftChannel[i] = mixOut;
        if (rightChannel != nullptr) rightChannel[i] = mixOut;
    }
}

const juce::String AIDrumMachineAudioProcessor::getName() const { return JucePlugin_Name; }
bool AIDrumMachineAudioProcessor::acceptsMidi() const { return false; }
bool AIDrumMachineAudioProcessor::producesMidi() const { return false; }
bool AIDrumMachineAudioProcessor::isMidiEffect() const { return false; }
double AIDrumMachineAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AIDrumMachineAudioProcessor::getNumPrograms() { return 1; }
int AIDrumMachineAudioProcessor::getCurrentProgram() { return 0; }
void AIDrumMachineAudioProcessor::setCurrentProgram(int index) {}
const juce::String AIDrumMachineAudioProcessor::getProgramName(int index) { return {}; }
void AIDrumMachineAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

bool AIDrumMachineAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* AIDrumMachineAudioProcessor::createEditor() { return new AIDrumMachineAudioProcessorEditor(*this); }
void AIDrumMachineAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {}
void AIDrumMachineAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AIDrumMachineAudioProcessor(); }