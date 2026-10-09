package com.airxonix.nativeport;

/** Pure-Java, testable projection of the Android touch stick onto legacy
 * four-direction AirXonix input. Values use a unit-radius circular control. */
final class VirtualStickDirection {
    static final int UP = 1, DOWN = 2, LEFT = 4, RIGHT = 8;
    static final float DEADZONE = 0.20f;
    private VirtualStickDirection() {}

    static int direction(float x, float y) {
        if ((x*x + y*y) < DEADZONE * DEADZONE) return 0;
        // Exactly one direction: the original player has no diagonal travel.
        return Math.abs(x) > Math.abs(y) ? (x < 0 ? LEFT : RIGHT)
                : (y < 0 ? UP : DOWN);
    }
}
