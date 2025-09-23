// Test script for renaming an existing Profile
void main(ScriptEngine@ e) {
    // Replace this with the actual profile ID you want to rename
    string existingProfileId = "18669b25937286b7b9d474a5";

    // New name you want to assign
    string newName = "RenamedProfile";

    // Call the wrapper to rename
    e.renameProfile(existingProfileId, newName);

    // Confirm action in console
    Print("Requested rename of profile " + existingProfileId + " to: " + newName);
}

