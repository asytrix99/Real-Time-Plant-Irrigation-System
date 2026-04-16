// Set MCG internal reference clock path to 1 MHz for peripheral timing.
void CLOCK_SetMCGIRCLK_1MHz()
{

    // Select internal reference clock
    MCG->C1 &= ~MCG_C1_CLKS_MASK;
    MCG->C1 |= MCG_C1_CLKS(1) | MCG_C1_IRCLKEN_MASK;

    // Use 2 MHz LIRC
    MCG->C2 |= MCG_C2_IRCS_MASK;

    // No division (DIV1)
    MCG->SC &= ~MCG_SC_FCRDIV_MASK;
    MCG->SC |= MCG_SC_FCRDIV(0);

    // Divide by 2 → final = 1 MHz
    MCG->MC &= ~MCG_MC_LIRC_DIV2_MASK;
    MCG->MC |= MCG_MC_LIRC_DIV2(1);
}