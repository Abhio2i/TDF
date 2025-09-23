// --------------------------------------------------
// Entity Types (int values):
//   0 = Platform
//   1 = Radio
//   2 = Sensor
//   3 = SpecialZone
//   4 = Weapon
//   5 = IFF
//   6 = Supply
//   7 = FixedPoint
//   8 = Formation
// --------------------------------------------------

void main(ScriptEngine@ e) {
    Print("=== Entity Finder Started ===");

    int targetType = 4; // Example: 0 = Platform
    array<Entity@>@ results = e.findEntitiesByType(targetType);

    Print("=== Entity Finder Finished ===");
}

