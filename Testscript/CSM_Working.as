void main(ScriptEngine@ e)
{
    ProfileCategaory@ platformProfile = e.getProfileByName("Platform");
    
    // ==================================================
    // Jet1: Agra → New Delhi (HAS RADIO)
    // ==================================================
    Platform@ jet1 = e.addEntityToPlatform(platformProfile, "Jet1");
    jet1.dynamicModel.moveSpeed = 1500;
    jet1.transform.setGeoCord(27.1767f, 78.0081f, 0.0f); // Agra
    jet1.trajectory.addWaypoint(27.1767f, 0.0f, 78.0081f); // Start
    jet1.trajectory.addWaypoint(28.6139f, 0.0f, 77.2090f); // New Delhi
    
    // ✅ Add Radio (emits communication signals)
    e.addRadioSubComponent(jet1, "Radio1");
    Print("✅ Jet1 equipped with Radio1");
    
    // ==================================================
    // Jet2: Gurugram → Hathras (HAS CSM)
    // ==================================================
    Platform@ jet2 = e.addEntityToPlatform(platformProfile, "Jet2");
    jet2.dynamicModel.moveSpeed = 1500;
    jet2.transform.setGeoCord(28.4595f, 77.0266f, 0.0f); // Gurugram
    jet2.trajectory.addWaypoint(28.4595f, 0.0f, 77.0266f); // Start
    jet2.trajectory.addWaypoint(27.5983f, 0.0f, 78.0506f); // End
    
    // ✅ Add CSM (detects radio communications)
    e.addSensorSubComponent(jet2, "CSM1", "CSM");
    Sensor@ jet2CSM = jet2.getSensorByName("CSM1");
    Print("✅ Jet2 equipped with CSM1");
    
    // ==================================================
    // Show CSM Display
    // ==================================================
    if (jet2CSM !is null) {
        e.selectEntityDisplay(jet2CSM);
        Print("✅ CSM display selected");
        e.sleep(1000);
    }
    
    // ==================================================
    // Simulation
    // ==================================================
    SimStart(e);
    Print("🚀 Simulation started!");
    Print("📻 CSM listening for radio communications...");
    
    for (int t = 0; t < 400; t++)
    {
        e.sleep(1000);
        
        if (jet2CSM !is null)
        {
            int csmCount = jet2CSM.getCSMTargetCount();
            
            // Debug output
            if (t % 10 == 0) {
                Print("[t=" + t + "] CSM targets: " + csmCount);
            }
            
            // ✅ AS SOON AS CSM DETECTS, STOP!
            if (csmCount > 0)
            {
                Print("═══════════════════════════════════════");
                Print("📻 CSM DETECTED RADIO! Count: " + csmCount);
                Print("═══════════════════════════════════════");
                
                // Switch display
                e.selectEntityDisplay(jet2CSM);
                e.sleep(1000);
                
                // Capture screenshots
                e.captureSensorScreenshot("csm_detection.png");
                Print("📸 Screenshots captured!");
                
                // Generate report
                Print("🛑 Stopping at t=" + t);
                e.generatePDFReport("CSM_Detection_Report.pdf");
                Print("✅ Report generated!");
                Print("═══════════════════════════════════════");
                
                // Break out of loop
                break;
            }
        }
    }
    
    SimPause(e);
    Print("✅ Simulation finished!");
}