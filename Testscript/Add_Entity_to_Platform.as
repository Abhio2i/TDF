void main(ScriptEngine@ e) {
    ProfileCategaory@ platformProfile = e.getProfileByName("Platform");

    Platform@ myPlatform = e.addEntityToPlatform(platformProfile, "Entity");

}
