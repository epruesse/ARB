// commandline option must follow the form:
// option=value


import java.util.*;


public class CommandLine
{

private HashMap hm;


public static void main(String[] args)
    {
        CommandLine lc = new CommandLine(args);
        lc.printArgs();
        System.out.println("Test: " + lc.getOptionValue("error"));

   }




public CommandLine(String[] arguments)
    {
        hm = new HashMap();
        int i = 0;
        String option;
        String value;
        while (i < arguments.length)
            {
                System.out.println(arguments[i]);
                if(arguments[i].indexOf('=') != -1)
                    {
                        option = arguments[i].substring(0,arguments[i].indexOf('='));
                        //System.out.println(option);
                        value = arguments[i].substring(arguments[i].indexOf('=')+1);
                        //System.out.println(value);
                    }
                else
                    {
                        option = arguments[i];
                        value = null;
                    }

                hm.put(option, value);
                i++;
            }


    }

public boolean getOption(String optionName)
    {
        if (hm.containsKey(optionName))
            {
                return true;
            }
        else
            {
                return false;
            }

    }


public String getOptionValue(String option)
    {
        return (String) hm.get(option);

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
