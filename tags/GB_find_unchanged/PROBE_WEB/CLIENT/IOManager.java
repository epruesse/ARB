//  ==================================================================== // 
//                                                                       // 
//    File      : IOManager.java                                         // 
//    Purpose   : encapsulates modal file dialogs for reading            // 
//                and saving of configuration settings and result        // 
//    Time-stamp: <Sat Mar/13/2004 17:40 MET Coder@ReallySoft.de>        // 
//                                                                       // 
//                                                                       // 
//  Coded by Lothar Richter + Ralf Westram in March 2004                 // 
//  Copyright Department of Microbiology (Technical University Munich)   // 
//                                                                       // 
//  Visit our web site at: http://www.arb-home.de/                       // 
//                                                                       // 
//  ==================================================================== // 

import java.awt.*;
import java.io.*;
import java.util.*;

public class IOManager{
    private Frame gui;
    private Map   defaultName4Type   = new HashMap();

    public IOManager(Frame controlledGui) {
        gui = controlledGui;
    }

    private void save_internal(String content_type, String content, String filename) throws Exception {
        try {
            FileWriter outfile = new FileWriter(filename);
            outfile.write(content);
            outfile.flush();
            outfile.close();

            defaultName4Type.put(content_type, filename);
        }
        catch (IOException e) {
            Toolkit.AbortWithError("while saving "+content_type+" to '"+filename+"': "+e.getMessage());
        }
    }


    private String get_default_name(String content_type) {
        String default_name = (String)defaultName4Type.get(content_type);

        if (default_name == null) {
            FileDialog fd = new FileDialog(gui,new String("Save " + content_type + " as"), FileDialog.SAVE);
            fd.setVisible(true);
            default_name = fd.getFile();
            if (default_name != null) {
                default_name = fd.getDirectory() + default_name;
            }
        }

        return default_name;
    }

    public void save(String content_type, String content) throws Exception {
        if (content.length() == 0) {
            Toolkit.AbortWithError("Nothing to save (empty content)");
        }
        String defaultName  = get_default_name(content_type);
        if (defaultName != null) {
            save_internal(content_type, content, defaultName);
        }
    }

    public void saveAsk(String content_type, String content) throws Exception {
        defaultName4Type.remove(content_type);
        save(content_type, content);
    }

    public void saveAs(String content_type, String content, String filename) throws Exception {
        save_internal(content_type, content, filename);
    }


}
