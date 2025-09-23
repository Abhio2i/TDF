void main(ScriptEngine@ e) {
   ProfileCategaory@ p = e.NewProfile('Profile');
   Folder@ f = e.addFolder(p.id, 'Air',true);
   Print("Create New Folder :"+f.name);
}

