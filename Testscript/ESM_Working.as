void main(ScriptEngine@ e)
{
    ProfileCategaory@ p = e.NewProfile("AircraftProfile");
    
    // ==================================================
    // Jet1: Agra → New Delhi (HAS RADAR - EMITS SIGNALS)
    // ==================================================
    Platform@ jet1 = e.addEntityToPlatform(p, "Jet1");
    jet1.dynamicModel.moveSpeed = 1500;
    jet1.transform.setGeoCord(27.1767f, 78.0081f, 0.0f); // Agra
    jet1.trajectory.addWaypoint(27.1767f, 0.0f, 78.0081f); // Start
    jet1.trajectory.addWaypoint(28.6139f, 0.0f, 77.2090f); // New Delhi
    e.addSensorSubComponent(jet1, "Radar", "Generic");
    Sensor@ jet1Radar = jet1.getSensorByName("Radar");
    
    // ==================================================
    // Jet2: Gurugram → Hathras (HAS ESM - DETECTS RADAR)
    // ==================================================
    Platform@ jet2 = e.addEntityToPlatform(p, "Jet2");
    jet2.dynamicModel.moveSpeed = 1500;
    jet2.transform.setGeoCord(28.4595f, 77.0266f, 0.0f); // Gurugram
    jet2.trajectory.addWaypoint(28.4595f, 0.0f, 77.0266f); // Start
    jet2.trajectory.addWaypoint(27.5983f, 0.0f, 78.0506f); // End
    e.addSensorSubComponent(jet2, "ESM1", "ESM");
    Sensor@ jet2ESM = jet2.getSensorByName("ESM1");
    
    // ==================================================
    // 🎯 AUTOMATED UI SETUP - Show ESM Display
    // ==================================================
    Print("🎯 Setting up automated display...");
    if (jet2ESM !is null) {
        e.selectEntityDisplay(jet2ESM);
        Print("✅ ESM display selected");
        Print("✅ Jet2 selected in hierarchy");
        e.sleep(1000);
    }
    
    // ==================================================
    // Simulation loop: check ESM detection every second
    // ==================================================
    SimStart(e);
    Print("🚀 Simulation started!");
    Print("📡 ESM listening for radar emissions...");
    
    bool detectionMade = false;
    
    for (int t = 0; t < 400; t++)
    {
        e.sleep(1000); // 1 second per tick
        
        // ✅ CHECK ESM DETECTION
        if (jet2ESM !is null && !detectionMade)
        {
            int esmCount = jet2ESM.getESMTargetCount();
            
            // ✅ AS SOON AS ESM DETECTS RADAR, STOP!
            if (esmCount > 0)
            {
                detectionMade = true; // Set flag immediately
                
                Print("═══════════════════════════════════════");
                Print("🎯 ESM DETECTED RADAR EMISSION!");
                Print("═══════════════════════════════════════");
                Print("Detection count: " + esmCount);
                
                // Switch to ESM display FIRST
                e.selectEntityDisplay(jet2ESM);
                Print("📡 Switched to ESM display");
                e.sleep(1000);
                
                // 📸 Capture DUAL screenshots (ESM Display + Canvas)
                e.captureSensorScreenshot("esm_detection.png");
                Print("📸 ESM detection screenshots captured!");
                
                // Stop simulation and generate report
                Print("🛑 Stopping simulation at t = " + t + " seconds.");
                Print("📊 Generating PDF report...");
                e.generatePDFReport("ESM_Detection_Report.pdf");
                Print("✅ Report generated: ESM_Detection_Report.pdf");
                Print("═══════════════════════════════════════");
                
                // Break out of loop
                break;
            }
        }
        
        // Safety exit if detection was made
        if (detectionMade) {
            break;
        }
    }
    
    SimPause(e);
    Print("✅ Simulation finished!");
    Print("🎯 Check the PDF for complete visual evidence!");
}