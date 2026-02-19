void main(ScriptEngine@ e)
{
   // City context (if you already use this)
   e.useCity("Bhopal");

    // Add a circle
   e.canvasAddCircle("Circle", 10.0);

    // Add rectangle (width, height)
   // e.canvasAddRectangle("Grid_Block", 40.0, 30.0);

    // Small delay so rectangle is created first
   //  e.sleep(3000);

    // Rotate rectangle by 45 degrees
   // e.rotateShape("TempRectangle_0", 45.0);

    // Move the newly created circle
   e.moveShape("TempCircle_0", 75.8481, 22.7375);

   e.showShapeHistory("TempCircle_0");

    //e.hideShapeHistory(); // Set city context

   // e.restoreShapeHistory("TempCircle_0");

    // Generate PDF report
    e.generatePDFReport("GIS_Report_History.pdf");

}
