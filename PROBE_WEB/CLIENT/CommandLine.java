// commandline option must follow the form:
// option=value


import java.util.*;


public class CommandLine
{
    private HashMap hm;
    private HashMap knownOptions;

    public CommandLine(String[] arguments, HashMap known) throws Exception {
        hm            = new HashMap();
        knownOptions  = known;
        int     i     = 0;
        String  option;
        String  value;

        while (i < arguments.length) {
            if(arguments[i].indexOf('=') != -1) {
                option = arguments[i].substring(0,arguments[i].indexOf('='));
                value  = arguments[i].substring(arguments[i].indexOf('=')+1);
            }
            else {
                option = arguments[i];
                value = null;
            }

            

            if (knownOptions.containsKey(option)) {
                hm.put(option, value);
            }
            else if (option.indexOf("help") != -1) {
                showHelp();
                System.exit(0); // sucessfully displayed help                
            }
            else {
                showHelpAndAbortWithError("Unknown option '"+option+"'");
            }
            i++;
        }
    }

    public void showHelpAndAbortWithError(String error) throws Exception {
        showHelp();
        Toolkit.AbortWithError(error);
    }

    public void showHelp() {
        System.out.println("Known options:");
        Set keys = knownOptions.keySet();
        for (Iterator it = keys.iterator(); it.hasNext(); ) {
            Object o = it.next();
            if (o != null) {
                String key = (String)o;
                System.out.println("    "+key+knownOptions.get(key));
            }
        }
    }

    private void checkLegalOption(String optionName) throws Exception {
        if (!knownOptions.containsKey(optionName)) {
            showHelpAndAbortWithError("Unknown option '"+optionName+"'");
        }
    }

    public boolean getOption(String optionName) throws Exception {
        checkLegalOption(optionName);
        return hm.containsKey(optionName);
    }

    public String getOptionValue(String optionName) throws Exception {
        checkLegalOption(optionName);
        return (String) hm.get(optionName);
    }

}
