void main(ScriptEngine@ e) {
   ProfileCategaory@ p = e.NewProfile('Profile');
   Folder@ f = e.addFolder(p.id, 'Air',true);
   Platform@ m = e.addEntity(f.id, 'entity',false);
   m.trajectory.addWaypoint(0,0,0);
   m.trajectory.addWaypoint(10,0,0);
   m.trajectory.addWaypoint(0,0,10);
   m.trajectory.addWaypoint(10,0,10);
   Print("Create New Entity :"+m.name);
}

