// commandline option must follow the form:
// option=value


import java.util.*;


public class CommandLine
{

private HashMap hm;
private HashMap knownOptions;


// public static void main(String[] args)
//     {
//         CommandLine lc = new CommandLine(args);
//         lc.printArgs();
//         System.out.println("Test: " + lc.getOptionValue("error"));

//    }




public CommandLine(String[] arguments, HashMap known)
    {
        hm            = new HashMap();
        knownOptions  = known;
        int     i     = 0;
        String  option;
        String  value;

        while (i < arguments.length)
        {
            // System.out.println("Arg="+arguments[i]);
            if(arguments[i].indexOf('=') != -1)
            {
                option = arguments[i].substring(0,arguments[i].indexOf('='));
                value  = arguments[i].substring(arguments[i].indexOf('=')+1);
            }
            else
            {
                option = arguments[i];
                value = null;
            }

            if (knownOptions.containsKey(option) || option == "help") {
                hm.put(option, value);
            }
            else {
                showHelpAndAbortWithError("Unknown option '"+option+"'");
            }
            i++;
        }

        if (hm.containsKey("help")) {
            showHelp();
            System.exit(0); // sucessfully displayed help
        }
    }

public void showHelpAndAbortWithError(String error)
    {
        showHelp();
        Toolkit.AbortWithError(error);
    }

public void showHelp()
    {
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

private void checkLegalOption(String optionName)
    {
        if (!knownOptions.containsKey(optionName)) {
            showHelpAndAbortWithError("Unknown option '"+optionName+"'");
        }
    }

public boolean getOption(String optionName)
    {
        checkLegalOption(optionName);
        if (hm.containsKey(optionName)) {
            return true;
        }
        else {
            return false;
        }
    }


public String getOptionValue(String optionName)
    {
        checkLegalOption(optionName);
        return (String) hm.get(optionName);
    }

public void printArgs()
    {
        Set keys = hm.keySet();
                System.out.println("OPTIONS:");
        for(Iterator it = keys.iterator();it.hasNext();)
            {
                System.out.println((String) it.next());
            }

        Set values = hm.entrySet();
                System.out.println("VALUES:");
        for(Iterator it = values.iterator();it.hasNext();)
            {
                Object o = it.next();
                if(o != null)
                    {
                        System.out.println( o);
                    }
                else
                    {
                        System.out.println("Object null");
                    }
            }


    }


}
