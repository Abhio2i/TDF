void main(ScriptEngine@ e) {
    ProfileCategaory@ platformProfile = e.getProfileByName("Platform");
    if (platformProfile is null) {
        Print("Platform profile not found!\n");
        return;
    }

    Platform@ myPlatform = e.addEntityToPlatform(platformProfile, "Entity");
    if (myPlatform is null) {
        Print("Failed to create platform.\n");
        return;
    }

}
