// class IOManager
// encapsulates modal file dialogs for
// reading and saving of configuration settings and result
// coded by Lothar Richter March 2004
// (c) ARB-Project


import java.awt.*;
import java.io.*;

public class IOManager{
    private String configFile = new String(".arbprobeconfig");
    private String resultFileName;
    private Frame gui;

    public IOManager(Frame controlledGui, String defaultConfigFile){
	if (defaultConfigFile.equals("")) defaultConfigFile = ".arbprobeconfig";
	gui = controlledGui;
    }


    public boolean readConfiguration(){return false;};
    public boolean saveConfiguration(){return false;};
    public boolean saveResults(String results){
	try {
	    if (resultFileName == null){ return saveResultsAs(results);}
	    FileWriter outfile = new FileWriter(resultFileName);
	    outfile.write(results);
	    outfile.flush();
	    outfile.close();
	    return true;
	}catch(IOException e){

	    return false;


	}



}
    public boolean saveResultsAs(String results){

	FileDialog fd = new FileDialog(gui,new String("Save Results As"),FileDialog.SAVE);
	fd.setVisible(true);

	resultFileName = fd.getDirectory() + fd.getFile();
	System.out.println(fd.getFile());
	System.out.println(fd.getDirectory());
	return saveResults(results);


    }



}
