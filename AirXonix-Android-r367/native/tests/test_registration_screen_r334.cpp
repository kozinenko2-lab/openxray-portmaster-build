#include "game/legacy_registration_trace.hpp"
#include <cassert>
#include <iostream>
int main(){
 static_assert(kLegacyRegistrationViolationTrace.routine==0x0040FF70u);
 static_assert(kLegacyRegistrationViolationTrace.integrityGlobal==0x025459B4u);
 static_assert(kLegacyRegistrationViolationTrace.excludedFromCleanroomRuntime);
 std::cout<<"registration screen r334 PASS\n";
}
