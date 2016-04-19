Known Issues
============

rev 1
-----

 * The 5V and ground on CON1 are reversed
   * **The D1, U1 and U3 WILL be damaged if the board is powered up**
   * Cutting the ground connections and the 5V line around the pads *must* be done
   before soldering. Then reversed connections will need to be added manually.
 * both SW1 and SW2 are rotated 90 degress from the correct connections.
   * Cutting diagonally opposed legs from the button before assembly will allow
   normal operation.
 * The ISCP slave select (SS) line is shared between IC1 and U2.
   * The ATmega *must* be programmed with a bootloader *before* soldering.
   * To program via ISCP, cut the RADIO_SS line between IC1 and U2.
 * The level converter (Q2, R3, R4) is too slow for the transmission speed required
 for the WS2812b LEDs.
   * Do not place Q1, R3 or R4. Make a bridge connection between pins 2–3 of Q2.
