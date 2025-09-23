void main(ScriptEngine@ e)
{
    string profileId = "1866991bf964ad257fd4da48";
    e.removeProfile(profileId);
    Print("Profile removed successfully if it existed.");
}

