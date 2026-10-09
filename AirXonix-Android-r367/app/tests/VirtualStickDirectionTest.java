package com.airxonix.nativeport;
public final class VirtualStickDirectionTest {
    private static void expect(int actual,int expected,String label){
        if (actual!=expected) throw new AssertionError(label+" actual="+actual+" expected="+expected);
    }
    public static void main(String[] unused){
        expect(VirtualStickDirection.direction(0f,0f),0,"idle");
        expect(VirtualStickDirection.direction(.15f,.08f),0,"deadzone");
        expect(VirtualStickDirection.direction(0f,-.9f),1,"up");
        expect(VirtualStickDirection.direction(0f,.9f),2,"down");
        expect(VirtualStickDirection.direction(-.9f,0f),4,"left");
        expect(VirtualStickDirection.direction(.9f,0f),8,"right");
        expect(VirtualStickDirection.direction(.8f,-.4f),8,"horizontal dominant");
        expect(VirtualStickDirection.direction(-.4f,.8f),2,"vertical dominant");
        expect(VirtualStickDirection.direction(.7f,-.7f),1,"ties vertical");
        System.out.println("9/9 virtual stick tests PASS");
    }
}
