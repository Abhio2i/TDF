void main(ScriptEngine@ e)
{
    e.useCity("Bhopal");

    e.addText("Bhopal HQ",  23.278665, 77.369882);
    e.sleep(1000);

    // Generate PDF report
    e.generatePDFReport("GIS_Report_Text.pdf");
  
}
