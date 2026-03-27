// ==============================================================================
// Source/PluginProcessor.cpp
// ==============================================================================
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

static constexpr InstrumentPatch P(int w, float fr, float pD, float pA, float aAt, float aDc, float ns, int fT, float fF, float fR, float dr, float vl) {
    return { w, fr, pD, pA, aAt, aDc, ns, fT, fF, fR, dr, vl };
}

static const std::array<InstrumentPatch, PATCH_MAX> patchLibrary = []() {
    std::array<InstrumentPatch, PATCH_MAX> arr{};

    // G0: Techno
    arr[G0_T0] = P(0, 48.0f, 8.0f, 5.0f, 1.0f, 25.0f, 0.05f, 0, 2000.0f, 1.0f, 2.5f, 1.1f);
    arr[G0_T1] = P(2, 180.0f, 6.0f, 1.5f, 1.0f, 18.0f, 0.8f, 0, 4000.0f, 1.2f, 3.0f, 0.9f);
    arr[G0_T2] = P(3, 800.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 5000.0f, 1.0f, 1.5f, 0.6f);
    arr[G0_T3] = P(3, 800.0f, 4.0f, 0.0f, 1.0f, 18.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G0_T4] = P(2, 120.0f, 6.0f, 0.0f, 2.0f, 20.0f, 1.0f, 2, 1500.0f, 1.0f, 2.5f, 0.9f);
    arr[G0_T5] = P(1, 400.0f, 4.0f, 0.0f, 1.0f, 25.0f, 0.6f, 1, 3000.0f, 1.0f, 1.5f, 0.6f);
    arr[G0_T6] = P(0, 100.0f, 10.0f, 1.5f, 1.0f, 22.0f, 0.0f, 0, 1200.0f, 1.0f, 2.0f, 1.0f);
    arr[G0_T7] = P(0, 100.0f, 0.0f, 0.0f, 1.0f, 15.0f, 1.0f, 2, 2000.0f, 1.0f, 2.5f, 0.8f);

    // G1: House
    arr[G1_T0] = P(0, 40.0f, 10.0f, 1.5f, 2.0f, 30.0f, 0.0f, 0, 800.0f, 1.0f, 2.0f, 1.1f);
    arr[G1_T1] = P(0, 400.0f, 4.0f, 1.5f, 1.0f, 8.0f, 0.1f, 2, 1200.0f, 3.0f, 2.0f, 1.0f);
    arr[G1_T2] = P(3, 900.0f, 2.0f, 0.0f, 1.0f, 7.0f, 0.9f, 1, 6000.0f, 1.0f, 1.5f, 0.5f);
    arr[G1_T3] = P(3, 800.0f, 4.0f, 0.0f, 1.0f, 20.0f, 0.9f, 1, 4500.0f, 1.0f, 1.5f, 0.6f);
    arr[G1_T4] = P(2, 150.0f, 5.0f, 0.0f, 3.0f, 15.0f, 0.8f, 2, 2000.0f, 1.0f, 2.0f, 0.9f);
    arr[G1_T5] = P(0, 250.0f, 6.0f, 1.5f, 1.0f, 18.0f, 0.0f, 2, 500.0f, 2.0f, 1.5f, 1.0f);
    arr[G1_T6] = P(0, 400.0f, 6.0f, 1.2f, 1.0f, 15.0f, 0.0f, 2, 800.0f, 2.0f, 1.5f, 1.0f);
    arr[G1_T7] = P(1, 300.0f, 8.0f, 0.5f, 2.0f, 15.0f, 0.2f, 2, 1200.0f, 4.0f, 2.5f, 0.9f);

    // G2: UKG
    arr[G2_T0] = P(2, 60.0f, 6.0f, 5.0f, 1.0f, 18.0f, 0.1f, 0, 3000.0f, 2.0f, 3.5f, 1.0f);
    arr[G2_T1] = P(0, 300.0f, 5.0f, 2.0f, 1.0f, 10.0f, 0.5f, 2, 1500.0f, 2.0f, 2.5f, 0.9f);
    arr[G2_T2] = P(3, 1000.0f, 2.0f, 0.0f, 1.0f, 5.0f, 1.0f, 1, 6000.0f, 1.5f, 1.5f, 0.5f);
    arr[G2_T3] = P(1, 500.0f, 4.0f, 0.0f, 1.0f, 25.0f, 0.6f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G2_T4] = P(2, 180.0f, 5.0f, 0.0f, 2.0f, 12.0f, 0.9f, 2, 2500.0f, 1.0f, 2.0f, 0.9f);
    arr[G2_T5] = P(0, 35.0f, 10.0f, 1.0f, 5.0f, 40.0f, 0.0f, 0, 400.0f, 1.0f, 3.0f, 1.2f);
    arr[G2_T6] = P(3, 600.0f, 4.0f, 0.5f, 1.0f, 8.0f, 0.1f, 2, 1000.0f, 3.0f, 1.5f, 0.8f);
    arr[G2_T7] = P(0, 700.0f, 2.0f, 0.0f, 1.0f, 6.0f, 0.8f, 1, 2000.0f, 1.0f, 1.5f, 0.8f);

    // G3: DnB
    arr[G3_T0] = P(3, 55.0f, 8.0f, 6.0f, 1.0f, 20.0f, 0.3f, 0, 4000.0f, 1.5f, 4.0f, 1.0f);
    arr[G3_T1] = P(0, 250.0f, 4.0f, 1.0f, 1.0f, 12.0f, 0.9f, 1, 800.0f, 1.0f, 3.5f, 0.9f);
    arr[G3_T2] = P(3, 900.0f, 2.0f, 0.0f, 1.0f, 4.0f, 1.0f, 1, 6000.0f, 1.0f, 1.5f, 0.5f);
    arr[G3_T3] = P(1, 450.0f, 4.0f, 0.0f, 1.0f, 22.0f, 0.5f, 1, 3500.0f, 1.0f, 1.5f, 0.6f);
    arr[G3_T4] = P(0, 350.0f, 4.0f, 1.5f, 1.0f, 8.0f, 0.2f, 2, 1200.0f, 2.0f, 2.5f, 0.9f);
    arr[G3_T5] = P(0, 180.0f, 8.0f, 1.2f, 1.0f, 15.0f, 0.1f, 0, 2000.0f, 1.0f, 2.5f, 1.0f);
    arr[G3_T6] = P(0, 120.0f, 8.0f, 1.5f, 1.0f, 18.0f, 0.1f, 0, 1500.0f, 1.0f, 2.5f, 1.0f);
    arr[G3_T7] = P(0, 30.0f, 5.0f, 1.0f, 3.0f, 50.0f, 0.0f, 0, 300.0f, 1.0f, 3.0f, 1.2f);

    // G4: Trap
    arr[G4_T0] = P(0, 45.0f, 15.0f, 4.0f, 1.0f, 45.0f, 0.0f, 0, 800.0f, 1.0f, 4.0f, 1.2f);
    arr[G4_T1] = P(0, 200.0f, 6.0f, 2.0f, 1.0f, 15.0f, 0.7f, 1, 600.0f, 1.0f, 2.5f, 0.9f);
    arr[G4_T2] = P(3, 1100.0f, 2.0f, 0.0f, 1.0f, 5.0f, 1.0f, 1, 6000.0f, 1.0f, 1.5f, 0.5f);
    arr[G4_T3] = P(3, 850.0f, 4.0f, 0.0f, 1.0f, 15.0f, 1.0f, 1, 4500.0f, 1.0f, 1.5f, 0.6f);
    arr[G4_T4] = P(2, 120.0f, 6.0f, 0.0f, 3.0f, 15.0f, 1.0f, 2, 1500.0f, 1.0f, 2.5f, 0.9f);
    arr[G4_T5] = P(3, 650.0f, 4.0f, 0.5f, 1.0f, 8.0f, 0.1f, 2, 1000.0f, 3.0f, 1.5f, 0.8f);
    arr[G4_T6] = P(0, 35.0f, 30.0f, 1.5f, 5.0f, 60.0f, 0.0f, 0, 300.0f, 1.0f, 4.5f, 1.3f);
    arr[G4_T7] = P(2, 1500.0f, 15.0f, 2.0f, 1.0f, 15.0f, 0.0f, 0, 2000.0f, 2.0f, 2.5f, 0.9f);

    // G5: Juke
    arr[G5_T0] = P(0, 50.0f, 12.0f, 3.0f, 1.0f, 35.0f, 0.0f, 0, 1000.0f, 1.0f, 3.0f, 1.2f);
    arr[G5_T1] = P(1, 180.0f, 8.0f, 1.5f, 1.0f, 18.0f, 0.8f, 1, 400.0f, 1.2f, 2.5f, 0.9f);
    arr[G5_T2] = P(3, 1000.0f, 2.0f, 0.0f, 1.0f, 4.0f, 1.0f, 1, 6000.0f, 1.0f, 1.5f, 0.5f);
    arr[G5_T3] = P(3, 800.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G5_T4] = P(2, 110.0f, 6.0f, 0.0f, 3.0f, 12.0f, 1.0f, 2, 1500.0f, 1.0f, 2.5f, 0.9f);
    arr[G5_T5] = P(0, 140.0f, 8.0f, 1.5f, 1.0f, 18.0f, 0.0f, 0, 1500.0f, 1.0f, 2.5f, 1.0f);
    arr[G5_T6] = P(3, 200.0f, 8.0f, 0.5f, 5.0f, 15.0f, 0.3f, 2, 1000.0f, 4.0f, 2.5f, 0.9f);
    arr[G5_T7] = P(1, 250.0f, 8.0f, -0.5f, 5.0f, 15.0f, 0.2f, 2, 1200.0f, 4.0f, 2.5f, 0.9f);

    // G6: IDM
    arr[G6_T0] = P(3, 60.0f, 8.0f, 6.0f, 1.0f, 20.0f, 0.3f, 0, 4000.0f, 1.5f, 4.0f, 0.9f);
    arr[G6_T1] = P(0, 300.0f, 4.0f, 1.0f, 1.0f, 8.0f, 0.9f, 1, 800.0f, 1.0f, 3.0f, 0.9f);
    arr[G6_T2] = P(3, 1200.0f, 1.0f, 0.0f, 1.0f, 3.0f, 1.0f, 1, 7000.0f, 1.0f, 1.5f, 0.5f);
    arr[G6_T3] = P(3, 1500.0f, 1.0f, 0.0f, 1.0f, 5.0f, 0.4f, 2, 4500.0f, 4.0f, 2.0f, 0.5f);
    arr[G6_T4] = P(0, 100.0f, 0.0f, 0.0f, 1.0f, 10.0f, 1.0f, 2, 3000.0f, 2.0f, 2.5f, 0.8f);
    arr[G6_T5] = P(3, 700.0f, 4.0f, 0.5f, 1.0f, 6.0f, 0.1f, 2, 1000.0f, 3.0f, 1.5f, 0.9f);
    arr[G6_T6] = P(3, 540.0f, 6.0f, 0.2f, 1.0f, 10.0f, 0.0f, 2, 800.0f, 4.0f, 2.5f, 0.8f);
    arr[G6_T7] = P(2, 500.0f, 12.0f, -0.8f, 1.0f, 15.0f, 1.0f, 2, 1000.0f, 8.0f, 4.0f, 0.8f);

    // G7: Dubstep
    arr[G7_T0] = P(3, 60.0f, 8.0f, 6.0f, 1.0f, 25.0f, 0.3f, 0, 4000.0f, 1.5f, 4.5f, 1.0f);
    arr[G7_T1] = P(2, 150.0f, 8.0f, 2.0f, 1.0f, 20.0f, 0.6f, 0, 5000.0f, 1.5f, 4.0f, 0.9f);
    arr[G7_T2] = P(3, 800.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G7_T3] = P(1, 400.0f, 4.0f, 0.0f, 1.0f, 20.0f, 0.6f, 1, 3000.0f, 1.0f, 1.5f, 0.6f);
    arr[G7_T4] = P(2, 120.0f, 6.0f, 0.0f, 3.0f, 20.0f, 1.0f, 2, 1500.0f, 1.0f, 3.0f, 0.9f);
    arr[G7_T5] = P(1, 45.0f, 15.0f, 0.5f, 5.0f, 30.0f, 0.2f, 0, 800.0f, 4.0f, 4.5f, 1.1f);
    arr[G7_T6] = P(2, 55.0f, 20.0f, -0.5f, 5.0f, 25.0f, 0.5f, 2, 1200.0f, 6.0f, 4.5f, 1.0f);
    arr[G7_T7] = P(0, 35.0f, 10.0f, 1.0f, 5.0f, 50.0f, 0.0f, 0, 300.0f, 1.0f, 3.5f, 1.2f);

    // G8: Afrobeat
    arr[G8_T0] = P(1, 55.0f, 8.0f, 2.0f, 1.0f, 15.0f, 0.2f, 0, 1500.0f, 1.5f, 1.8f, 1.0f);
    arr[G8_T1] = P(0, 350.0f, 4.0f, 1.5f, 1.0f, 10.0f, 0.2f, 2, 1500.0f, 2.0f, 1.5f, 0.9f);
    arr[G8_T2] = P(0, 200.0f, 0.0f, 0.0f, 8.0f, 10.0f, 1.0f, 2, 3000.0f, 1.5f, 1.5f, 0.5f);
    arr[G8_T3] = P(0, 250.0f, 0.0f, 0.0f, 8.0f, 12.0f, 1.0f, 2, 3500.0f, 1.5f, 1.5f, 0.5f);
    arr[G8_T4] = P(0, 2000.0f, 2.0f, 0.0f, 1.0f, 5.0f, 0.0f, 3, 0.0f, 0.0f, 1.5f, 0.9f);
    arr[G8_T5] = P(0, 250.0f, 6.0f, 1.5f, 1.0f, 15.0f, 0.0f, 2, 500.0f, 2.0f, 2.0f, 1.0f);
    arr[G8_T6] = P(0, 180.0f, 8.0f, 2.0f, 1.0f, 20.0f, 0.1f, 2, 400.0f, 2.0f, 2.0f, 1.0f);
    arr[G8_T7] = P(3, 900.0f, 6.0f, 0.2f, 1.0f, 18.0f, 0.0f, 2, 1200.0f, 4.0f, 2.5f, 0.8f);

    // G9: Gqom
    arr[G9_T0] = P(0, 42.0f, 12.0f, 3.0f, 1.0f, 30.0f, 0.0f, 0, 1000.0f, 1.0f, 3.5f, 1.2f);
    arr[G9_T1] = P(0, 200.0f, 6.0f, 2.0f, 1.0f, 12.0f, 0.7f, 1, 600.0f, 1.0f, 2.5f, 0.9f);
    arr[G9_T2] = P(3, 800.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G9_T3] = P(3, 800.0f, 4.0f, 0.0f, 1.0f, 15.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G9_T4] = P(2, 110.0f, 6.0f, 0.0f, 3.0f, 15.0f, 1.0f, 2, 1500.0f, 1.0f, 2.5f, 0.9f);
    arr[G9_T5] = P(0, 100.0f, 12.0f, 2.0f, 1.0f, 25.0f, 0.0f, 0, 1000.0f, 1.0f, 3.0f, 1.1f);
    arr[G9_T6] = P(0, 80.0f, 15.0f, 2.0f, 1.0f, 30.0f, 0.0f, 0, 800.0f, 1.0f, 3.0f, 1.1f);
    arr[G9_T7] = P(3, 200.0f, 8.0f, 0.5f, 5.0f, 15.0f, 0.3f, 2, 1000.0f, 4.0f, 2.5f, 0.9f);

    // G10: Amapiano
    arr[G10_T0] = P(0, 60.0f, 8.0f, 1.5f, 2.0f, 35.0f, 0.0f, 0, 500.0f, 1.0f, 3.5f, 1.2f);
    arr[G10_T1] = P(0, 400.0f, 4.0f, 1.5f, 1.0f, 8.0f, 0.1f, 2, 1200.0f, 3.0f, 2.0f, 1.0f);
    arr[G10_T2] = P(0, 200.0f, 0.0f, 0.0f, 8.0f, 10.0f, 1.0f, 2, 3000.0f, 1.5f, 1.5f, 0.5f);
    arr[G10_T3] = P(3, 800.0f, 4.0f, 0.0f, 1.0f, 15.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G10_T4] = P(2, 130.0f, 6.0f, 0.0f, 3.0f, 15.0f, 1.0f, 2, 1500.0f, 1.0f, 2.0f, 0.9f);
    arr[G10_T5] = P(0, 250.0f, 6.0f, 1.5f, 1.0f, 15.0f, 0.0f, 2, 500.0f, 2.0f, 2.0f, 1.0f);
    arr[G10_T6] = P(3, 600.0f, 4.0f, 0.5f, 1.0f, 8.0f, 0.1f, 2, 1000.0f, 3.0f, 1.5f, 0.9f);
    arr[G10_T7] = P(0, 1800.0f, 5.0f, 0.2f, 5.0f, 15.0f, 0.0f, 3, 0.0f, 0.0f, 1.5f, 0.8f);

    // G11: Indian Cls
    arr[G11_T0] = P(0, 70.0f, 20.0f, 2.0f, 1.0f, 25.0f, 0.0f, 0, 800.0f, 2.0f, 1.5f, 1.1f);
    arr[G11_T1] = P(0, 280.0f, 8.0f, 2.0f, 1.0f, 15.0f, 0.1f, 2, 600.0f, 2.5f, 2.0f, 1.0f);
    arr[G11_T2] = P(0, 400.0f, 6.0f, 1.2f, 1.0f, 12.0f, 0.0f, 2, 800.0f, 2.0f, 2.0f, 1.0f);
    arr[G11_T3] = P(3, 1500.0f, 4.0f, 0.0f, 1.0f, 15.0f, 0.2f, 2, 5000.0f, 4.0f, 1.5f, 0.6f);
    arr[G11_T4] = P(0, 250.0f, 0.0f, 0.0f, 8.0f, 15.0f, 1.0f, 2, 4000.0f, 1.5f, 1.5f, 0.5f);
    arr[G11_T5] = P(0, 150.0f, 10.0f, 1.5f, 1.0f, 20.0f, 0.0f, 0, 1000.0f, 1.5f, 1.8f, 1.0f);
    arr[G11_T6] = P(0, 110.0f, 12.0f, 1.8f, 1.0f, 22.0f, 0.0f, 0, 800.0f, 1.5f, 1.8f, 1.0f);
    arr[G11_T7] = P(3, 220.0f, 8.0f, 0.5f, 5.0f, 15.0f, 0.3f, 2, 1000.0f, 4.0f, 2.0f, 0.9f);

    // G12: Samba
    arr[G12_T0] = P(0, 45.0f, 12.0f, 1.5f, 3.0f, 35.0f, 0.0f, 0, 600.0f, 1.0f, 2.0f, 1.1f);
    arr[G12_T1] = P(0, 250.0f, 4.0f, 1.0f, 1.0f, 10.0f, 0.9f, 1, 800.0f, 1.0f, 2.5f, 0.9f);
    arr[G12_T2] = P(0, 350.0f, 6.0f, 1.2f, 1.0f, 12.0f, 0.1f, 2, 800.0f, 2.0f, 2.0f, 1.0f);
    arr[G12_T3] = P(0, 200.0f, 0.0f, 0.0f, 8.0f, 10.0f, 1.0f, 2, 3000.0f, 1.5f, 1.5f, 0.5f);
    arr[G12_T4] = P(3, 700.0f, 4.0f, 0.5f, 1.0f, 8.0f, 0.1f, 2, 1200.0f, 3.0f, 1.5f, 0.9f);
    arr[G12_T5] = P(3, 850.0f, 6.0f, 0.2f, 1.0f, 18.0f, 0.0f, 2, 1200.0f, 4.0f, 2.5f, 0.8f);
    arr[G12_T6] = P(0, 300.0f, 25.0f, -2.0f, 2.0f, 25.0f, 0.1f, 0, 1500.0f, 2.0f, 2.5f, 1.0f);
    arr[G12_T7] = P(0, 200.0f, 8.0f, 1.5f, 1.0f, 15.0f, 0.1f, 2, 1000.0f, 2.0f, 2.0f, 1.0f);

    // G13: Reggaeton
    arr[G13_T0] = P(2, 60.0f, 6.0f, 5.0f, 1.0f, 18.0f, 0.1f, 0, 3000.0f, 2.0f, 3.0f, 1.0f);
    arr[G13_T1] = P(0, 220.0f, 5.0f, 1.0f, 1.0f, 12.0f, 0.8f, 1, 800.0f, 1.0f, 3.0f, 1.0f);
    arr[G13_T2] = P(3, 850.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 5000.0f, 1.0f, 1.5f, 0.6f);
    arr[G13_T3] = P(3, 850.0f, 4.0f, 0.0f, 1.0f, 15.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G13_T4] = P(2, 120.0f, 6.0f, 0.0f, 3.0f, 15.0f, 1.0f, 2, 1500.0f, 1.0f, 2.0f, 0.9f);
    arr[G13_T5] = P(0, 280.0f, 8.0f, 1.5f, 1.0f, 18.0f, 0.1f, 2, 800.0f, 2.0f, 2.0f, 1.0f);
    arr[G13_T6] = P(3, 600.0f, 4.0f, 0.5f, 1.0f, 8.0f, 0.1f, 2, 1000.0f, 3.0f, 1.5f, 0.9f);
    arr[G13_T7] = P(3, 200.0f, 8.0f, 0.5f, 5.0f, 15.0f, 0.3f, 2, 1000.0f, 4.0f, 2.5f, 0.9f);

    // G14: Gamelan
    arr[G14_T0] = P(2, 100.0f, 15.0f, 0.5f, 1.0f, 40.0f, 0.2f, 0, 800.0f, 3.0f, 2.0f, 1.0f);
    arr[G14_T1] = P(2, 150.0f, 12.0f, 0.5f, 1.0f, 30.0f, 0.1f, 0, 1000.0f, 2.0f, 1.8f, 1.0f);
    arr[G14_T2] = P(0, 120.0f, 12.0f, 3.0f, 1.0f, 18.0f, 0.0f, 2, 300.0f, 3.0f, 2.5f, 1.1f);
    arr[G14_T3] = P(3, 540.0f, 6.0f, 0.2f, 1.0f, 12.0f, 0.0f, 2, 800.0f, 4.0f, 2.5f, 0.8f);
    arr[G14_T4] = P(0, 300.0f, 12.0f, 0.0f, 1.0f, 18.0f, 0.05f, 0, 1000.0f, 1.0f, 1.5f, 1.0f);
    arr[G14_T5] = P(2, 250.0f, 10.0f, 0.5f, 1.0f, 25.0f, 0.1f, 0, 1200.0f, 2.0f, 1.8f, 1.0f);
    arr[G14_T6] = P(3, 600.0f, 4.0f, 0.5f, 1.0f, 8.0f, 0.1f, 2, 1000.0f, 3.0f, 1.5f, 0.9f);
    arr[G14_T7] = P(0, 50.0f, 10.0f, 1.0f, 2.0f, 30.0f, 0.0f, 0, 400.0f, 1.0f, 2.0f, 1.1f);

    // G15: Funk
    arr[G15_T0] = P(1, 55.0f, 8.0f, 2.0f, 1.0f, 15.0f, 0.2f, 0, 1500.0f, 1.5f, 1.8f, 1.0f);
    arr[G15_T1] = P(1, 220.0f, 8.0f, 1.2f, 1.0f, 18.0f, 0.6f, 0, 6000.0f, 1.0f, 2.0f, 0.9f);
    arr[G15_T2] = P(3, 800.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G15_T3] = P(3, 800.0f, 4.0f, 0.0f, 1.0f, 15.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G15_T4] = P(2, 120.0f, 6.0f, 0.0f, 3.0f, 15.0f, 1.0f, 2, 1500.0f, 1.0f, 2.0f, 0.9f);
    arr[G15_T5] = P(0, 120.0f, 8.0f, 1.5f, 1.0f, 18.0f, 0.0f, 0, 1500.0f, 1.0f, 2.5f, 1.0f);
    arr[G15_T6] = P(0, 250.0f, 6.0f, 1.5f, 1.0f, 15.0f, 0.0f, 2, 500.0f, 2.0f, 2.0f, 1.0f);
    arr[G15_T7] = P(3, 600.0f, 0.0f, 0.0f, 3.0f, 12.0f, 0.8f, 1, 5000.0f, 2.0f, 2.0f, 0.5f);

    // G16: New Jack Swing
    arr[G16_T0] = P(2, 60.0f, 6.0f, 5.0f, 1.0f, 15.0f, 0.1f, 0, 3000.0f, 2.0f, 3.5f, 1.0f);
    arr[G16_T1] = P(2, 150.0f, 8.0f, 2.0f, 1.0f, 20.0f, 0.6f, 0, 5000.0f, 1.5f, 3.5f, 0.9f);
    arr[G16_T2] = P(3, 900.0f, 2.0f, 0.0f, 1.0f, 5.0f, 1.0f, 1, 6000.0f, 1.0f, 1.5f, 0.5f);
    arr[G16_T3] = P(3, 900.0f, 4.0f, 0.0f, 1.0f, 15.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G16_T4] = P(2, 100.0f, 6.0f, 0.0f, 3.0f, 15.0f, 1.0f, 2, 1500.0f, 1.0f, 2.5f, 0.9f);
    arr[G16_T5] = P(0, 150.0f, 8.0f, 1.5f, 1.0f, 18.0f, 0.0f, 0, 1500.0f, 1.0f, 2.5f, 1.0f);
    arr[G16_T6] = P(0, 100.0f, 8.0f, 1.2f, 1.0f, 20.0f, 0.0f, 0, 1200.0f, 1.0f, 2.5f, 1.0f);
    arr[G16_T7] = P(2, 440.0f, 8.0f, 0.0f, 1.0f, 12.0f, 0.0f, 0, 1500.0f, 2.0f, 2.5f, 0.9f);

    // G17: Neo Soul
    arr[G17_T0] = P(0, 50.0f, 8.0f, 1.0f, 2.0f, 20.0f, 0.0f, 0, 500.0f, 1.0f, 1.5f, 1.0f);
    arr[G17_T1] = P(0, 400.0f, 4.0f, 1.5f, 1.0f, 8.0f, 0.1f, 2, 1200.0f, 3.0f, 2.0f, 1.0f);
    arr[G17_T2] = P(3, 800.0f, 3.0f, 0.0f, 2.0f, 8.0f, 1.0f, 1, 3500.0f, 1.0f, 1.2f, 0.6f);
    arr[G17_T3] = P(1, 400.0f, 4.0f, 0.0f, 1.0f, 20.0f, 0.6f, 1, 3000.0f, 1.0f, 1.5f, 0.6f);
    arr[G17_T4] = P(0, 800.0f, 4.0f, 0.0f, 1.0f, 8.0f, 0.8f, 1, 2000.0f, 1.0f, 1.5f, 0.9f);
    arr[G17_T5] = P(0, 80.0f, 12.0f, 2.0f, 1.0f, 20.0f, 0.0f, 0, 1000.0f, 1.0f, 2.5f, 1.1f);
    arr[G17_T6] = P(0, 200.0f, 0.0f, 0.0f, 8.0f, 10.0f, 1.0f, 2, 3000.0f, 1.5f, 1.5f, 0.5f);
    arr[G17_T7] = P(3, 150.0f, 8.0f, 1.0f, 1.0f, 15.0f, 0.5f, 2, 800.0f, 2.0f, 4.0f, 0.8f);

    // G18: Hip Hop
    arr[G18_T0] = P(3, 60.0f, 8.0f, 6.0f, 1.0f, 20.0f, 0.3f, 0, 4000.0f, 1.5f, 4.0f, 0.9f);
    arr[G18_T1] = P(2, 150.0f, 8.0f, 2.0f, 1.0f, 20.0f, 0.6f, 0, 5000.0f, 1.5f, 3.5f, 0.9f);
    arr[G18_T2] = P(3, 800.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 4000.0f, 1.0f, 2.0f, 0.6f);
    arr[G18_T3] = P(3, 800.0f, 4.0f, 0.0f, 1.0f, 15.0f, 1.0f, 1, 4000.0f, 1.0f, 2.0f, 0.6f);
    arr[G18_T4] = P(2, 100.0f, 6.0f, 0.0f, 3.0f, 15.0f, 1.0f, 2, 1500.0f, 1.0f, 2.5f, 0.9f);
    arr[G18_T5] = P(3, 600.0f, 4.0f, 0.5f, 1.0f, 8.0f, 0.1f, 2, 1000.0f, 3.0f, 1.5f, 0.9f);
    arr[G18_T6] = P(2, 100.0f, 40.0f, 2.0f, 5.0f, 30.0f, 0.8f, 2, 2000.0f, 2.0f, 2.5f, 0.9f);
    arr[G18_T7] = P(3, 150.0f, 8.0f, 1.0f, 1.0f, 15.0f, 0.5f, 2, 800.0f, 2.0f, 4.0f, 0.8f);

    // G19: Math Rock
    arr[G19_T0] = P(1, 55.0f, 8.0f, 2.0f, 1.0f, 15.0f, 0.2f, 0, 1500.0f, 1.5f, 1.8f, 1.0f);
    arr[G19_T1] = P(1, 220.0f, 8.0f, 1.2f, 1.0f, 18.0f, 0.6f, 0, 6000.0f, 1.0f, 2.0f, 0.9f);
    arr[G19_T2] = P(3, 800.0f, 3.0f, 0.0f, 1.0f, 6.0f, 1.0f, 1, 4000.0f, 1.0f, 1.5f, 0.6f);
    arr[G19_T3] = P(1, 400.0f, 4.0f, 0.0f, 1.0f, 20.0f, 0.6f, 1, 3000.0f, 1.0f, 1.5f, 0.6f);
    arr[G19_T4] = P(1, 220.0f, 8.0f, 1.2f, 1.0f, 18.0f, 0.6f, 0, 6000.0f, 1.0f, 1.0f, 0.5f);
    arr[G19_T5] = P(0, 120.0f, 8.0f, 1.5f, 1.0f, 18.0f, 0.0f, 0, 1500.0f, 1.0f, 2.5f, 1.0f);
    arr[G19_T6] = P(0, 80.0f, 12.0f, 2.0f, 1.0f, 20.0f, 0.0f, 0, 1000.0f, 1.0f, 2.5f, 1.1f);
    arr[G19_T7] = P(2, 300.0f, 8.0f, 0.0f, 1.0f, 30.0f, 0.9f, 1, 2000.0f, 1.0f, 2.5f, 0.6f);

    // G20: Prog Metal
    arr[G20_T0] = P(0, 85.0f, 4.0f, 8.0f, 0.5f, 12.0f, 0.2f, 0, 6000.0f, 1.0f, 3.5f, 1.1f);
    arr[G20_T1] = P(2, 190.0f, 8.0f, 2.0f, 1.0f, 18.0f, 0.5f, 0, 4000.0f, 1.5f, 4.0f, 1.0f);
    arr[G20_T2] = P(3, 1200.0f, 2.0f, 0.0f, 1.0f, 6.0f, 0.8f, 1, 6000.0f, 2.0f, 3.0f, 0.8f);
    arr[G20_T3] = P(3, 800.0f, 5.0f, 0.0f, 1.0f, 15.0f, 0.9f, 1, 4000.0f, 1.0f, 1.5f, 0.7f);
    arr[G20_T4] = P(0, 220.0f, 10.0f, 1.5f, 1.0f, 25.0f, 0.0f, 0, 2000.0f, 1.0f, 2.0f, 1.1f);
    arr[G20_T5] = P(0, 140.0f, 12.0f, 1.5f, 1.0f, 28.0f, 0.0f, 0, 1500.0f, 1.0f, 2.0f, 1.1f);
    arr[G20_T6] = P(0, 85.0f, 15.0f, 1.5f, 1.0f, 35.0f, 0.0f, 0, 1000.0f, 1.0f, 2.0f, 1.1f);
    arr[G20_T7] = P(2, 400.0f, 10.0f, 0.0f, 1.0f, 40.0f, 1.0f, 1, 3000.0f, 1.0f, 3.0f, 0.7f);

    // G21: Minimalism
    arr[G21_T0] = P(2, 100.0f, 6.0f, 0.0f, 3.0f, 15.0f, 1.0f, 2, 1500.0f, 1.0f, 2.0f, 0.9f);
    arr[G21_T1] = P(0, 800.0f, 4.0f, 0.0f, 1.0f, 8.0f, 0.8f, 1, 2000.0f, 1.0f, 1.5f, 0.9f);
    arr[G21_T2] = P(0, 300.0f, 12.0f, 0.0f, 1.0f, 18.0f, 0.05f, 0, 1000.0f, 1.0f, 1.5f, 1.0f);
    arr[G21_T3] = P(0, 400.0f, 12.0f, 0.0f, 1.0f, 18.0f, 0.05f, 0, 1200.0f, 1.0f, 1.5f, 1.0f);
    arr[G21_T4] = P(3, 600.0f, 4.0f, 0.5f, 1.0f, 8.0f, 0.1f, 2, 1000.0f, 3.0f, 1.5f, 0.9f);
    arr[G21_T5] = P(0, 50.0f, 8.0f, 1.0f, 2.0f, 20.0f, 0.0f, 0, 500.0f, 1.0f, 1.5f, 1.0f);
    arr[G21_T6] = P(0, 400.0f, 4.0f, 1.5f, 1.0f, 8.0f, 0.1f, 2, 1200.0f, 3.0f, 2.0f, 1.0f);
    arr[G21_T7] = P(0, 450.0f, 4.0f, 1.5f, 1.0f, 8.0f, 0.1f, 2, 1200.0f, 3.0f, 2.0f, 1.0f);

    for (int i = 176; i < 184; ++i) {
        float f = static_cast<float>(130.81 * std::pow(1.05946, (i - 176) * 2));
        arr[i] = P(2, f, 12.0f, 0.0f, 1.0f, 25.0f, 0.0f, 0, 1500.0f, 2.0f, 2.0f, 1.0f);
    }
    arr[M_ARP] = P(1, 440.0f, 5.0f, 0.0f, 0.5f, 15.0f, 0.0f, 0, 2500.0f, 1.5f, 1.5f, 1.0f);

    return arr;
    }();

static const std::array<GenreDefinition, 24> genreTable = { {
    { 4, 4, 125, 135, {"909 Kick", "909 Snare", "CHH", "OHH", "Clap", "Ride", "Tom", "Noise FX"},
      {G0_T0, G0_T1, G0_T2, G0_T3, G0_T4, G0_T5, G0_T6, G0_T7},
      {{1,2,4,0}, {2,4,0,0}, {4,8,0,0}, {4,8,0,0}, {2,4,0,0}, {3,4,5,0}, {3,5,7,0}, {2,4,8,0}}, {0,0,0,0,0,0,0,0}, {0,0,2,2,0,5,5,10} },
    { 4, 4, 120, 126, {"Deep Kick", "Rimshot", "Shuff Hat", "Open Hat", "Clap", "Conga", "Bongo", "Vocal Chop"},
      {G1_T0, G1_T1, G1_T2, G1_T3, G1_T4, G1_T5, G1_T6, G1_T7},
      {{1,2,4,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,5,0,0}, {4,6,8,0}, {2,3,4,0}}, {-2,0, 5,0,-2, -5, -5, 0}, {2,5, 15,5, 5, 10, 10, 15} },
    { 4, 4, 130, 138, {"Punch Kick", "Snare/Rim", "Garage Hat", "Ride", "Clap", "Sub Bass", "Perc 1", "Perc 2"},
      {G2_T0, G2_T1, G2_T2, G2_T3, G2_T4, G2_T5, G2_T6, G2_T7},
      {{2,3,4,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {2,4,0,0}, {2,3,4,0}, {3,5,0,0}, {5,7,0,0}}, {-5,5, 10,5, 0, -10, 0, 0}, {5,15, 25,15, 10, 10, 15, 15} },
    { 4, 4, 165, 175, {"Heavy Kick", "Tight Snr", "Fast Hat", "Ride", "Break Rim", "Break 1", "Break 2", "Sub"},
      {G3_T0, G3_T1, G3_T2, G3_T3, G3_T4, G3_T5, G3_T6, G3_T7},
      {{2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {4,8,0,0}, {2,4,0,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}}, {0,0, -5,-5, 0, -10, -10, 0}, {2,2, 5,5, 5, 10, 10, 5} },
    { 4, 4, 135, 150, {"808 Kick", "808 Snare", "Roll Hat", "Open Hat", "Clap", "Perc", "808 Bass", "FX"},
      {G4_T0, G4_T1, G4_T2, G4_T3, G4_T4, G4_T5, G4_T6, G4_T7},
      {{2,4,0,0}, {2,4,0,0}, {4,6,8,0}, {2,4,0,0}, {2,4,0,0}, {4,8,0,0}, {2,4,0,0}, {2,4,0,0}}, {0,0, 0,0, 0, 0, 0, 0}, {0,0, 0,0, 0, 0, 0, 0} },
    { 4, 4, 160, 160, {"Juke Kick", "Snare", "Fast Hat", "Hat 2", "Clap", "Tom", "Vocal 1", "Vocal 2"},
      {G5_T0, G5_T1, G5_T2, G5_T3, G5_T4, G5_T5, G5_T6, G5_T7},
      {{3,4,6,0}, {3,4,6,0}, {4,6,8,0}, {4,6,8,0}, {2,4,0,0}, {3,5,6,0}, {3,4,5,0}, {4,6,7,0}}, {-5,-5, -5,-5, 0, -10, -5, -5}, {5,5, 5,5, 5, 10, 15, 15} },
    { 4, 4, 140, 180, {"Glitch Kick", "Drill Snr", "Hat 1", "Hat 2", "Noise", "Perc 1", "Perc 2", "Glitch FX"},
      {G6_T0, G6_T1, G6_T2, G6_T3, G6_T4, G6_T5, G6_T6, G6_T7},
      {{4,5,7,0}, {4,6,8,0}, {5,7,9,0}, {6,8,9,0}, {3,5,7,0}, {5,7,9,0}, {4,6,8,0}, {3,5,7,0}}, {-10,-10, -15,-15, -20, -20, -20, -20}, {10,10, 15,15, 20, 20, 20, 20} },
    { 4, 4, 140, 150, {"Stomp Kick", "Fat Snare", "Hat", "Ride", "Clap", "Wobble", "Growl", "Sub"},
      {G7_T0, G7_T1, G7_T2, G7_T3, G7_T4, G7_T5, G7_T6, G7_T7},
      {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,8,0}, {2,4,0,0}, {1,2,4,0}}, {0,0, 0,0, 0, 0, 0, 0}, {0,0, 5,5, 0, 10, 5, 0} },
    { 4, 4, 95, 115, {"Acoustic Kick", "Snare", "Shaker 1", "Shaker 2", "Clave", "Conga", "Djembe", "Agogo"},
      {G8_T0, G8_T1, G8_T2, G8_T3, G8_T4, G8_T5, G8_T6, G8_T7},
      {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {3,4,0,0}, {3,4,5,0}, {3,4,6,0}, {2,3,4,0}}, {0,0, 3,3, 5, 5, 5, 5}, {5,10, 15,15, 20, 20, 20, 20} },
    { 4, 4, 98, 108, {"Heavy Kick", "Snare", "Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Chant"},
      {G9_T0, G9_T1, G9_T2, G9_T3, G9_T4, G9_T5, G9_T6, G9_T7},
      {{3,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}}, {-5,0, 0,0, -5, -5, -5, 0}, {0,5, 10,5, 0, 10, 10, 10} },
    { 4, 4, 110, 115, {"Log Drum", "Snare/Rim", "Shaker", "Open Hat", "Clap", "Conga", "Woodblock", "Whistle"},
      {G10_T0, G10_T1, G10_T2, G10_T3, G10_T4, G10_T5, G10_T6, G10_T7},
      {{3,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {2,3,4,0}}, {0,0, 5,0, 0, 5, 5, 0}, {10,5, 15,5, 5, 15, 15, 10} },
    { 7, 8, 40, 60, {"Bayan", "Dayan", "Tabla", "Manjira", "Ghungroo", "Dholak 1", "Dholak 2", "Vocal"},
      {G11_T0, G11_T1, G11_T2, G11_T3, G11_T4, G11_T5, G11_T6, G11_T7},
      {{2,3,4,0}, {2,3,4,5}, {2,3,4,5}, {2,3,4,0}, {3,4,5,6}, {2,3,4,0}, {2,3,4,0}, {1,2,3,0}}, {-5,-5, -5, -2, -2, -5, -5, -10}, {5,5, 5, 5, 5, 5, 5, 10} },
    { 4, 4, 90, 120, {"Surdo", "Caixa", "Pandeiro", "Ganza", "Tamborim", "Agogo", "Cuica", "Repique"},
      {G12_T0, G12_T1, G12_T2, G12_T3, G12_T4, G12_T5, G12_T6, G12_T7},
      {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {4,6,0,0}, {3,4,0,0}, {3,4,0,0}, {3,4,0,0}, {4,6,0,0}}, {0,5, 5,5, 5, 5, 5, 5}, {5,15, 20,20, 20, 20, 20, 20} },
    { 4, 4, 90, 105, {"Kick", "Snare (Tresillo)", "Hat", "Open Hat", "Clap", "Timbales", "Perc", "Vocal FX"},
      {G13_T0, G13_T1, G13_T2, G13_T3, G13_T4, G13_T5, G13_T6, G13_T7},
      {{2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,0,0}, {2,4,0,0}}, {0,-6, 0,0, 0, 0, 0, 0}, {5,0, 5,5, 5, 10, 10, 10} },
    { 8, 4, 40, 55, {"Gong", "Kempul", "Kendang", "Bonang", "Saron", "Kenong", "Kethuk", "Slenthem"},
      {G14_T0, G14_T1, G14_T2, G14_T3, G14_T4, G14_T5, G14_T6, G14_T7},
      {{1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}}, {-10,-10, -5,-5, -5, -10, -5, -10}, {10,10, 10,10, 10, 10, 10, 10} },
    { 4, 4, 100, 115, {"Kick", "Snare", "Hi-Hat", "Open Hat", "Clap", "Tom", "Conga", "Tambourine"},
      {G15_T0, G15_T1, G15_T2, G15_T3, G15_T4, G15_T5, G15_T6, G15_T7},
      {{2,4,0,0}, {2,4,0,0}, {4,6,0,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {3,4,6,0}, {4,6,0,0}}, {0,0, 5,0, 0, 0, 5, 5}, {5,10, 20,5, 5, 10, 15, 20} },
    { 4, 4, 100, 112, {"Punch Kick", "Snare", "Swing Hat", "Open Hat", "Clap", "Tom 1", "Tom 2", "Orch Hit"},
      {G16_T0, G16_T1, G16_T2, G16_T3, G16_T4, G16_T5, G16_T6, G16_T7},
      {{2,4,0,0}, {2,4,0,0}, {6,12,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}, {2,4,0,0}}, {0,0, 15,0, 0, 0, 0, 0}, {5,5, 25,5, 5, 5, 5, 5} },
    { 4, 4, 80, 95, {"Soft Kick", "Rimshot", "Loose Hat", "Ride", "Snap", "Tom", "Shaker", "Vinyl FX"},
      {G17_T0, G17_T1, G17_T2, G17_T3, G17_T4, G17_T5, G17_T6, G17_T7},
      {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {3,4,0,0}, {2,4,0,0}, {2,3,4,0}, {4,6,0,0}, {1,2,0,0}}, {-5, 10, 20, 10, 5, 0, 15, 0}, {2, 25, 40, 25, 20, 10, 30, 0} },
    { 4, 4, 85, 95, {"Gritty Kick", "Fat Snare", "Hi-Hat", "Open Hat", "Clap", "Perc", "Scratch", "Sample"},
      {G18_T0, G18_T1, G18_T2, G18_T3, G18_T4, G18_T5, G18_T6, G18_T7},
      {{2,4,0,0}, {2,4,0,0}, {3,4,6,0}, {2,4,0,0}, {2,4,0,0}, {3,4,0,0}, {2,4,0,0}, {1,2,4,0}}, {2, 5, 5, 0, 0, 0, 0, 0}, {8, 12, 15, 5, 5, 10, 5, 0} },
    { 5, 4, 120, 160, {"Kick", "Snare", "Hi-Hat", "Ride", "Ghost Snr", "Tom 1", "Tom 2", "Crash"},
      {G19_T0, G19_T1, G19_T2, G19_T3, G19_T4, G19_T5, G19_T6, G19_T7},
      {{2,3,4,0}, {2,3,4,0}, {4,5,6,0}, {3,4,5,0}, {4,6,8,0}, {3,4,5,0}, {3,4,5,0}, {1,2,0,0}}, {0,0, 0,0, 0, 0, 0, 0}, {5,5, 5,5, 5, 5, 5, 5} },
    { 13, 8, 60, 85, {"Kick (Click)", "Snare (Fat)", "Max Stax", "Hat Bark", "High Tom", "Mid Tom", "Floor Tom", "Splash"},
      {G20_T0, G20_T1, G20_T2, G20_T3, G20_T4, G20_T5, G20_T6, G20_T7},
      {{2,4,0,0}, {2,4,0,0}, {4,6,8,0}, {2,4,0,0}, {3,4,5,0}, {3,4,5,0}, {3,4,5,0}, {1,2,0,0}}, {0, 2, 0, 4, 1, 2, 3, 0}, {0, 2, 0, 6, 1, 2, 3, 0} },
    { 12, 8, 60, 75, {"Clap 1", "Clap 2", "Marimba 1", "Marimba 2", "Woodblock", "Pulse", "Phase 1", "Phase 2"},
      {G21_T0, G21_T1, G21_T2, G21_T3, G21_T4, G21_T5, G21_T6, G21_T7},
      {{1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}, {1,2,0,0}}, {0,0, 0,0, 0, 0, -20, 20}, {0,0, 0,0, 0, 0, -20, 20} },
    { 4, 4, 120, 150, {"Node C", "Node D", "Node F", "Node G", "Node A", "Node C^", "Node D^", "Node F^"},
      {PLUCK_1, PLUCK_2, PLUCK_3, PLUCK_4, PLUCK_5, PLUCK_6, PLUCK_7, PLUCK_8},
      {{2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}, {2,3,5,7}}, {0,0,0,0,0,0,0,0}, {0,0,0,0,0,0,0,0} },
    { 4, 4, 120, 150, {"Chaos C", "Chaos D", "Chaos F", "Chaos G", "Chaos A", "Chaos C^", "Chaos D^", "Chaos F^"},
      {PLUCK_1, PLUCK_2, PLUCK_3, PLUCK_4, PLUCK_5, PLUCK_6, PLUCK_7, PLUCK_8},
      {{1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}, {1,3,6,9}}, {-20,-20,-20,-20,-20,-20,-20,-20}, {20,20,20,20,20,20,20,20} }
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
    initializeUserTunings();
}

AIDrumMachineAudioProcessor::~AIDrumMachineAudioProcessor() {}

void AIDrumMachineAudioProcessor::initializeUserTunings() {
    for (int g = 0; g < 24; ++g) {
        userTuning[g].tempo.min = genreTable[g].minTempo;
        userTuning[g].tempo.max = genreTable[g].maxTempo;
        userTuning[g].tempoLocked = false;
        for (int t = 0; t < 8; ++t) {
            userTuning[g].allowedTimeSigs[t] = (t == 0);
            userTuning[g].tracks[t].divLocked = false;
            for (int d = 0; d < 8; ++d) userTuning[g].tracks[t].allowedDivs[d] = (d == 0 || d == 3);
            userTuning[g].tracks[t].cmplx.min = 20; userTuning[g].tracks[t].cmplx.max = 50;
            userTuning[g].tracks[t].cmplxLocked = false;
            userTuning[g].tracks[t].entrp.min = 10; userTuning[g].tracks[t].entrp.max = 50;
            userTuning[g].tracks[t].entrpLocked = false;
            userTuning[g].tracks[t].shift.min = 0; userTuning[g].tracks[t].shift.max = 0;
            userTuning[g].tracks[t].shiftLocked = false;
        }
        for (int f = 0; f < 4; ++f) userTuning[g].allowedFills[f] = true;
    }

    // =========================================================================
    // ⬇️ DUMP TO CLIPBOARD で取得したコードをここにペーストしてください ⬇️
    // =========================================================================



    // =========================================================================
    // ⬆️ ペーストエリアここまで ⬆️
    // =========================================================================
}

const GenreDefinition& AIDrumMachineAudioProcessor::getGenreDef(int index) {
    if (index < 0 || index >= 24) return genreTable[0];
    return genreTable[index];
}

const InstrumentPatch& AIDrumMachineAudioProcessor::getPatch(PatchID id) {
    if (id < 0 || id >= PATCH_MAX) return patchLibrary[id];
    return patchLibrary[id];
}

juce::String AIDrumMachineAudioProcessor::getNoteName(int trackIndex) const {
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
    GenreTuning& tuning = userTuning[genre];
    bool isAlgorithmMode = (genre >= 22);
    bool isSpecialEnsemble = (genre == 14 || genre == 21);

    int fillBar = fillBarTarget.load();
    bool isArp = arpMode.load();
    bool isMono = arpMono.load();
    int curScale = arpScale.load();

    if (!tuning.tempoLocked && !tempoLocked.load() && !isSyncEnabled.load() && !isArp) {
        int tMin = std::min(tuning.tempo.min, tuning.tempo.max);
        int tMax = std::max(tuning.tempo.min, tuning.tempo.max);
        int newBpm = random.nextInt(juce::Range<int>(tMin, tMax + 1));
        internalTempo.store((double)newBpm);
    }

    int num = timeSigNumerator.load();
    int den = timeSigDenominator.load();

    if (!isArp) {
        int tsChoices[8];
        int numTsChoices = 0;
        for (int i = 0; i < 8; ++i) {
            if (tuning.allowedTimeSigs[i]) tsChoices[numTsChoices++] = i;
        }
        if (numTsChoices > 0) {
            int pickedTs = tsChoices[random.nextInt(numTsChoices)];
            num = tuning.timeSigOptions[pickedTs].num;
            den = tuning.timeSigOptions[pickedTs].den;
            timeSigNumerator.store(num);
            timeSigDenominator.store(den);
        }
    }

    int maxDiv = (den == 16) ? 2 : ((den == 8) ? 4 : 8);
    int bars = globalBarCount.load();

    if (isArp) {
        int arpPreset = currentGenre.load();
        std::vector<int> usedNotes;

        for (int trk = 0; trk < 8; ++trk) {
            if (trackLocked[trk]) {
                usedNotes.push_back(trackOctaveUI[trk] * 12 + trackDegreeUI[trk]);
                continue;
            }

            for (int j = 0; j < 1024; ++j) drumPatternUI[trk][j] = 0;
            if (!trackDivLocked[trk]) trackDivisionsUI[trk] = (arpPreset == 7) ? ((trk % 4) + 2) : 4;

            int degree = trackDegreeUI[trk];
            int octave = trackOctaveUI[trk];
            int attempts = 0;
            bool unique = false;

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
                    unique = true; usedNotes.push_back(noteVal);
                }
                attempts++;
            }
            trackDegreeUI[trk] = degree;
            trackOctaveUI[trk] = octave;
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
                currentTrk = nextTrk; tracksToHit.push_back(currentTrk); break;
            }

            if (arpPreset != 7) {
                if (isMono && tracksToHit.size() > 0) {
                    if (!stepOccupied[j]) {
                        if (!trackLocked[tracksToHit[0]]) drumPatternUI[tracksToHit[0]][j] = vel;
                        stepOccupied[j] = true;
                    }
                }
                else {
                    for (int t : tracksToHit) { if (!trackLocked[t]) drumPatternUI[t][j] = vel; }
                }
            }
        }

        if (arpPreset == 7) {
            for (int trk = 0; trk < 8; ++trk) {
                if (trackLocked[trk]) continue;
                int tDiv = trackDivisionsUI[trk];
                for (int j = 0; j < tDiv * num * bars; ++j) {
                    if (j % tDiv == 0 && random.nextInt(100) < 60) drumPatternUI[trk][j] = 70 + random.nextInt(30);
                }
            }
        }
    }
    else {
        int fillChoices[4];
        int numFillChoices = 0;
        for (int i = 0; i < 4; ++i) if (tuning.allowedFills[i]) fillChoices[numFillChoices++] = i;
        int fillTypology = numFillChoices > 0 ? fillChoices[random.nextInt(numFillChoices)] : 0;

        for (int trk = 0; trk < 8; ++trk) {
            if (trackLocked[trk]) continue;

            TrackTuning& tt = tuning.tracks[trk];

            if (!tt.divLocked && !trackDivLocked[trk]) {
                std::vector<int> candidates;
                for (int i = 0; i < 8; ++i) {
                    if (tt.allowedDivs[i]) candidates.push_back(i + 1);
                }
                int newDiv = 4;
                if (!candidates.empty()) newDiv = candidates[random.nextInt((int)candidates.size())];
                if (newDiv > maxDiv) newDiv = maxDiv;
                trackDivisionsUI[trk] = newDiv;
            }

            if (!tt.cmplxLocked && !trackCmplxLocked[trk]) {
                int cMin = std::min(tt.cmplx.min, tt.cmplx.max);
                int cMax = std::max(tt.cmplx.min, tt.cmplx.max);
                trackComplexity[trk] = isAlgorithmMode ? 50 : random.nextInt(juce::Range<int>(cMin, cMax + 1));
            }
            if (!tt.entrpLocked && !trackEntrpLocked[trk]) {
                int eMin = std::min(tt.entrp.min, tt.entrp.max);
                int eMax = std::max(tt.entrp.min, tt.entrp.max);
                trackEntropy[trk] = random.nextInt(juce::Range<int>(eMin, eMax + 1));
            }
            if (!tt.shiftLocked && !trackShiftLocked[trk]) {
                int sMin = std::min(tt.shift.min, tt.shift.max);
                int sMax = std::max(tt.shift.min, tt.shift.max);
                trackShiftUI[trk] = random.nextInt(juce::Range<int>(sMin, sMax + 1));
            }

            int div = trackDivisionsUI[trk];
            int n = div * num * bars;
            int offset = random.nextInt(juce::Range<int>(0, juce::jmax(1, n)));

            int cmplx = trackComplexity[trk];
            int entrp = trackEntropy[trk];

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

                if (isFillPortion && !isAlgorithmMode) {
                    int localStep = stepInBar - div * (num - 2);
                    if (localStep < 0) localStep = stepInBar;

                    if (fillTypology == 0) {
                        isNegativeAnchor = true;
                        int prime = 7;
                        int targetTrk = ((localStep * prime) + currentBarOfStep) % 8;
                        if (entrp > 30) targetTrk = (targetTrk + (entrp % 5)) % 8;
                        if (trk == targetTrk) { isAnchor = true; anchorVel = 75 + random.nextInt(25); isNegativeAnchor = false; }
                    }
                    else if (fillTypology == 1) {
                        isNegativeAnchor = true;
                        if (trk != 0 && trk != 5 && trk != 6) {
                            int targetTrk = 1 + ((localStep * 3) % 4);
                            if (trk == targetTrk) { isAnchor = true; anchorVel = 60 + random.nextInt(40); isNegativeAnchor = false; }
                            if (stepInBar == (div * num) - (div > 1 ? div / 2 : 1) && (trk == 1 || trk == 7)) { isAnchor = true; anchorVel = 100; isNegativeAnchor = false; }
                        }
                    }
                    else if (fillTypology == 2) {
                        isNegativeAnchor = true;
                        int fillLen = div * 2;
                        if (fillLen > 0) {
                            int k = 5;
                            if ((localStep * k) % fillLen < k) {
                                int trkChoices[] = { 1, 2, 4, 5, 7 };
                                int chosenTrk = trkChoices[(localStep + currentBarOfStep) % 5];
                                if (trk == chosenTrk) { isAnchor = true; anchorVel = 80 + random.nextInt(20); isNegativeAnchor = false; }
                            }
                        }
                    }
                    else {
                        isNegativeAnchor = true;
                        if (trk != 0 && trk != 6) {
                            int activeTrk = 1 + (localStep / (div > 0 ? div : 1)) % 4;
                            if (trk == activeTrk) {
                                if (stepInBar % (div > 1 ? div / 2 : 1) == 0) {
                                    isAnchor = true; isNegativeAnchor = false;
                                    float prog = (float)(stepInBar % (div * 2)) / (float)(juce::jmax(1, div * 2));
                                    anchorVel = 50 + (int)(50.0f * prog);
                                }
                            }
                        }
                    }
                }
                else if (!isAlgorithmMode) {
                    switch (genre) {
                    case 0: case 1:
                        if (trk == 0) {
                            if (stepInBar % div == 0) {
                                isAnchor = true;
                                if ((currentBarOfStep % 2 != 0) && beatInBar == num - 1 && random.nextInt(100) < 15) { isAnchor = false; isNegativeAnchor = true; }
                            }
                        }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; }
                        if (trk == 2) { if (stepInBar % div == div / 2) isAnchor = true; }
                        if (trk == 3) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 5 || trk == 6) { if (stepInBar % div == (div > 1 ? div - 1 : 0) && random.nextInt(100) < 40) { isAnchor = true; anchorVel = 60; } }
                        if (trk == 7) { if (currentBarOfStep % 4 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 80; } }
                        break;

                    case 2:
                        if (trk == 0) {
                            if (stepInBar == 0) isAnchor = true;
                            int barType = currentBarOfStep % 4;
                            if (barType == 0 || barType == 2) { if (stepInBar == div * 2 + div / 2) isAnchor = true; }
                            else if (barType == 1) { if (stepInBar == div * 3 + div / 2) isAnchor = true; }
                            else { if (stepInBar == div + div / 2 || stepInBar == div * 3 + div / 2) isAnchor = true; }
                        }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 2) {
                            if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 70; }
                            if (stepInBar % div == div - 1 && random.nextInt(100) < 30) { isAnchor = true; anchorVel = 50; }
                        }
                        if (trk == 3) { if (stepInBar == div * 2 + div / 2) { isAnchor = true; anchorVel = 85; } }
                        if (trk == 5) { if (stepInBar == 0 || stepInBar == div * 2 + div / 2) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 6 || trk == 7) { if (stepInBar == div + div / 2 || stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 60; } }
                        break;

                    case 4: case 7:
                        if (trk == 0) {
                            if (stepInBar == 0) isAnchor = true;
                            if (genre == 4 && stepInBar == div + div / 2) isAnchor = true;
                            if (genre == 7 && stepInBar == div * 2 + div / 2) isAnchor = true;
                        }
                        if (trk == 1 || trk == 4) { if (stepInBar == div * 2) { isAnchor = true; anchorVel = 100; } }
                        if (genre == 4 && trk == 2) {
                            if (stepInBar % (div > 1 ? div / 2 : 1) == 0) isAnchor = true;
                            if ((currentBarOfStep % 2 != 0) && beatInBar == 3 && random.nextInt(100) < 50) { isAnchor = true; anchorVel = 80; }
                        }
                        if (trk == 3) { if (stepInBar == 0 || stepInBar == div * 2) { isAnchor = true; anchorVel = 80; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 70; } }
                        if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 100; } }
                        break;

                    case 3:
                        if (trk == 0) {
                            if (stepInBar == 0) isAnchor = true;
                            if (currentBarOfStep % 2 == 0 && stepInBar == div * 2 + div / 2) isAnchor = true;
                            if (currentBarOfStep % 2 != 0 && stepInBar == div * 3) isAnchor = true;
                        }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 2) { if (stepInBar % div == 0) isAnchor = true; }
                        if (trk == 3) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 70; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div + div / 2) { isAnchor = true; anchorVel = 60; } }
                        if (trk == 7) { if (stepInBar == 0) { isAnchor = true; anchorVel = 95; } }
                        break;

                    case 15: case 16:
                        if (trk == 0) {
                            if (stepInBar == 0) isAnchor = true;
                            if (currentBarOfStep % 2 == 0 && stepInBar == div * 2 + div / 2) isAnchor = true;
                            if (currentBarOfStep % 2 != 0 && stepInBar == div * 2 + div - 1) isAnchor = true;
                        }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; }
                        if (trk == 2) {
                            if (stepInBar % div == 0) isAnchor = true;
                            else if (random.nextInt(100) < 30) { isAnchor = true; anchorVel = 40; }
                        }
                        if (trk == 3) { if (stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 80; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div - 1 || stepInBar == div * 3 - 1) { isAnchor = true; anchorVel = 50; } }
                        if (trk == 7) { if (currentBarOfStep % 2 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 90; } }
                        break;

                    case 17: case 18:
                        if (trk == 0) {
                            if (stepInBar == 0) isAnchor = true;
                            if (stepInBar == div * 2 + div / 2 && random.nextInt(100) < 70) isAnchor = true;
                        }
                        if (trk == 1 || trk == 4) { if (stepInBar == div || stepInBar == div * 3) isAnchor = true; }
                        if (trk == 3) { if (stepInBar % (div * 2) == div) { isAnchor = true; anchorVel = 60; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div * 2 - 1) { isAnchor = true; anchorVel = 40; } }
                        if (trk == 7) { if (currentBarOfStep % 4 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 85; } }
                        break;

                    case 8: case 12:
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 2 + div / 2) isAnchor = true; }
                        if (trk == 1 || trk == 4) {
                            if (stepInBar == 0 || stepInBar == div + div / 2 || stepInBar == div * 2 + div || stepInBar == div * 3 || stepInBar == div * 4) { isAnchor = true; anchorVel = 85; }
                        }
                        if (trk == 2) { if (stepInBar % div == 0) isAnchor = true; }
                        if (trk == 3) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 75; } }
                        if (trk >= 5 && trk <= 7) { if (stepInBar == div - 1 || stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 80; } }
                        break;

                    case 9: case 10: case 13:
                        if (trk == 0) { if (stepInBar % div == 0) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == div - 1 || stepInBar == div * 2 + div / 2) isAnchor = true; }
                        if (trk == 2 || trk == 3) { if (stepInBar % (div > 1 ? div / 2 : 1) == 0) { isAnchor = true; anchorVel = 70; } }
                        if (trk == 5) { if (stepInBar == div + div / 2) { isAnchor = true; anchorVel = 80; } }
                        if (genre == 10 && (trk == 6 || trk == 7)) {
                            if (currentBarOfStep % 2 != 0 && (stepInBar == div * 2 + div / 2 || stepInBar == div * 3 + div / 2)) { isAnchor = true; anchorVel = 100; }
                        }
                        else if (trk == 6 || trk == 7) {
                            if (stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 80; }
                        }
                        break;

                    case 11: case 14:
                        if (trk == 0) {
                            if (currentBarOfStep % 4 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 100; }
                            else { isNegativeAnchor = true; }
                        }
                        if (trk >= 1 && trk <= 4) { if (stepInBar % (div * 2) == 0) { isAnchor = true; anchorVel = 70; } }
                        if (trk >= 5 && trk <= 7) { if (stepInBar % div == div / 2) { isAnchor = true; anchorVel = 60; } }
                        break;

                    case 19:
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 3) isAnchor = true; }
                        if (trk == 1 || trk == 4) { if (stepInBar == div * 2 || stepInBar == div * 4) isAnchor = true; }
                        if (trk == 2) {
                            if (stepInBar % div == 0) isAnchor = true;
                            if (random.nextInt(100) < 20) { isAnchor = true; anchorVel = 50; }
                        }
                        if (trk == 3) { if (stepInBar == div * 3 + div / 2) { isAnchor = true; anchorVel = 80; } }
                        if (trk == 5 || trk == 6) { if (stepInBar == div + div / 2 || stepInBar == div * 4 + div / 2) { isAnchor = true; anchorVel = 75; } }
                        if (trk == 7) { if (currentBarOfStep % 2 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 90; } }
                        break;

                    case 20:
                        if (trk == 0) { if (stepInBar == 0 || stepInBar == div * 3 || stepInBar == div * 5) isAnchor = true; }
                        if (trk == 1) { if (stepInBar == div * 2 || stepInBar == div * 4 || stepInBar == div * 6) isAnchor = true; }
                        if (trk == 2) { if (stepInBar % (div > 1 ? div / 2 : 1) == 0) { isAnchor = true; anchorVel = 70; } }
                        if (trk == 3) { if (stepInBar == div * 2 + (div > 1 ? div / 2 : 0) || stepInBar == div * 4 + (div > 1 ? div / 2 : 0)) { isAnchor = true; anchorVel = 85; } }
                        if (trk == 4) { if (currentBarOfStep % 2 == 1 && stepInBar == div * (num - 1)) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 5) { if (currentBarOfStep % 2 == 1 && stepInBar == div * (num - 1) + (div > 1 ? div / 2 : 0)) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 6) { if (currentBarOfStep % 2 == 1 && stepInBar == div * num - 1) { isAnchor = true; anchorVel = 90; } }
                        if (trk == 7) { if (currentBarOfStep % 2 == 0 && stepInBar == 0) { isAnchor = true; anchorVel = 100; } }
                        break;

                    case 21:
                        if (trk == 0) { if (stepInBar % (div * 3) == 0) isAnchor = true; }
                        if (trk == 1) { if ((stepInBar + div) % (div * 3) == 0) isAnchor = true; }
                        if (trk >= 2 && trk <= 4) { if (stepInBar % 2 == 0) isAnchor = true; }
                        if (trk >= 5 && trk <= 7) { if (stepInBar % 3 == 0) { isAnchor = true; anchorVel = 70; } }
                        break;

                    default:
                        if (trk == 0 && stepInBar == 0) isAnchor = true;
                        if ((trk == 1 || trk == 4) && (stepInBar == div || stepInBar == div * 3) && num >= 4) isAnchor = true;
                        if (trk >= 5) { if (stepInBar == div * 2 + div / 2) { isAnchor = true; anchorVel = 60; } }
                        break;
                    }

                    if (trk == 0 && !isAnchor && !isNegativeAnchor) {
                        if (stepInBar % div == div - 1) {
                            if (cmplx > 0 && entrp > 0) {
                                if (random.nextInt(100) < (entrp / 2)) {
                                    drumPatternUI[trk][j] = random.nextInt(juce::Range<int>(40, 75));
                                    continue;
                                }
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
                else if (cmplx > 0) {
                    bool isHit = (((j + offset) * k) % n) < k;
                    if (entrp > 0 && random.nextInt(100) < (entrp / 3)) isHit = !isHit;
                    vel = isHit ? random.nextInt(juce::Range<int>(40, 90 + (entrp / 10))) : 0;
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
                            InstrumentPatch p = getPatch(isArp ? M_ARP : def.trackPatches[trk]);
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

void AIDrumMachineAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::XmlElement xml("AIDrumMachineState");
    xml.setAttribute("currentGenre", currentGenre.load());
    xml.setAttribute("globalBarCount", globalBarCount.load());

    auto* tuningsXml = new juce::XmlElement("UserTunings");
    for (int g = 0; g < 24; ++g) {
        auto* gXml = new juce::XmlElement("Genre");
        gXml->setAttribute("id", g);
        gXml->setAttribute("tMin", userTuning[g].tempo.min);
        gXml->setAttribute("tMax", userTuning[g].tempo.max);
        gXml->setAttribute("tLock", userTuning[g].tempoLocked);

        juce::String tsStr, fStr;
        for (int i = 0; i < 8; ++i) tsStr += userTuning[g].allowedTimeSigs[i] ? "1" : "0";
        for (int i = 0; i < 4; ++i) fStr += userTuning[g].allowedFills[i] ? "1" : "0";
        gXml->setAttribute("ts", tsStr);
        gXml->setAttribute("fills", fStr);

        for (int t = 0; t < 8; ++t) {
            auto* tXml = new juce::XmlElement("Track");
            tXml->setAttribute("id", t);

            juce::String dStr;
            for (int d = 0; d < 8; ++d) dStr += userTuning[g].tracks[t].allowedDivs[d] ? "1" : "0";
            tXml->setAttribute("divs", dStr);

            tXml->setAttribute("dLck", userTuning[g].tracks[t].divLocked);
            tXml->setAttribute("cMin", userTuning[g].tracks[t].cmplx.min);
            tXml->setAttribute("cMax", userTuning[g].tracks[t].cmplx.max);
            tXml->setAttribute("cLck", userTuning[g].tracks[t].cmplxLocked);
            tXml->setAttribute("eMin", userTuning[g].tracks[t].entrp.min);
            tXml->setAttribute("eMax", userTuning[g].tracks[t].entrp.max);
            tXml->setAttribute("eLck", userTuning[g].tracks[t].entrpLocked);
            tXml->setAttribute("sMin", userTuning[g].tracks[t].shift.min);
            tXml->setAttribute("sMax", userTuning[g].tracks[t].shift.max);
            tXml->setAttribute("sLck", userTuning[g].tracks[t].shiftLocked);
            gXml->addChildElement(tXml);
        }
        tuningsXml->addChildElement(gXml);
    }
    xml.addChildElement(tuningsXml);

    copyXmlToBinary(xml, destData);
}

void AIDrumMachineAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName("AIDrumMachineState")) {
        currentGenre.store(xmlState->getIntAttribute("currentGenre", 0));
        globalBarCount.store(xmlState->getIntAttribute("globalBarCount", 4));

        if (auto* tuningsXml = xmlState->getChildByName("UserTunings")) {
            for (auto* gXml : tuningsXml->getChildIterator()) {
                int g = gXml->getIntAttribute("id", -1);
                if (g >= 0 && g < 24) {
                    userTuning[g].tempo.min = gXml->getIntAttribute("tMin", 120);
                    userTuning[g].tempo.max = gXml->getIntAttribute("tMax", 120);
                    userTuning[g].tempoLocked = gXml->getBoolAttribute("tLock", false);

                    juce::String tsStr = gXml->getStringAttribute("ts", "10000000");
                    for (int i = 0; i < 8; ++i) userTuning[g].allowedTimeSigs[i] = (tsStr[i] == '1');

                    juce::String fStr = gXml->getStringAttribute("fills", "1111");
                    for (int i = 0; i < 4; ++i) userTuning[g].allowedFills[i] = (fStr[i] == '1');

                    for (auto* tXml : gXml->getChildIterator()) {
                        int t = tXml->getIntAttribute("id", -1);
                        if (t >= 0 && t < 8) {
                            juce::String dStr = tXml->getStringAttribute("divs", "10010000");
                            for (int d = 0; d < 8; ++d) userTuning[g].tracks[t].allowedDivs[d] = (dStr[d] == '1');

                            userTuning[g].tracks[t].divLocked = tXml->getBoolAttribute("dLck", false);

                            userTuning[g].tracks[t].cmplx.min = tXml->getIntAttribute("cMin", 20);
                            userTuning[g].tracks[t].cmplx.max = tXml->getIntAttribute("cMax", 50);
                            userTuning[g].tracks[t].cmplxLocked = tXml->getBoolAttribute("cLck", false);

                            userTuning[g].tracks[t].entrp.min = tXml->getIntAttribute("eMin", 10);
                            userTuning[g].tracks[t].entrp.max = tXml->getIntAttribute("eMax", 50);
                            userTuning[g].tracks[t].entrpLocked = tXml->getBoolAttribute("eLck", false);

                            userTuning[g].tracks[t].shift.min = tXml->getIntAttribute("sMin", 0);
                            userTuning[g].tracks[t].shift.max = tXml->getIntAttribute("sMax", 0);
                            userTuning[g].tracks[t].shiftLocked = tXml->getBoolAttribute("sLck", false);
                        }
                    }
                }
            }
        }
        uiNeedsUpdate.store(true);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AIDrumMachineAudioProcessor(); }