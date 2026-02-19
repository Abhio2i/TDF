void main(ScriptEngine@ e)
{
    e.useCity("Bhopal");

    e.addText("Bhopal HQ", 77.369882, 23.278665);
    e.sleep(1000);

    // Generate PDF report
    e.generatePDFReport("GIS_Report_Text.pdf");
  
}
