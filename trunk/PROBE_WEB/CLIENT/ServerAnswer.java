import java.util.*;

public class ServerAnswer
{
private HashMap map;
private String  error_message;

public ServerAnswer(String answer) {
    map                  = new HashMap();
    error_message        = "";
    boolean format_error = false;

    int start = 0;
    int nl    = answer.indexOf('\n');

    while (nl != -1) {
        String line = answer.substring(start, nl);
        int    eq   = line.indexOf('=');
        if (eq !=  -1) {
            String key   = line.substring(0, eq);
            String value = line.substring(eq+1);
            map.put(key, value);
            System.out.println("key='"+key+"' value='"+value+"'");
        }
        else {
            format_error = true;
        }

        start = nl+1;
        nl    = answer.indexOf('\n', start);
    }

    if (format_error) {
        if (hasKey("result") && !getValue("result").equals("ok")) {
            format_error   = false;
            error_message  = getValue("message");
        }
        else {
            error_message = "Server problem ("+answer+")";
        }
    }
    else {
        if (hasKey("result")) {
            String result = getValue("result");
            if (!result.equals("ok")) {
                error_message  = getValue("message");
            }
        }
        else {
            error_message = "Server problem (no result found)";
        }
    }
}

public boolean hasError() {
    return !error_message.equals("");
}

public String getError() {
    if (!hasError()) return "";
    return error_message;
}

public boolean hasKey(String key) {
    return map.containsKey(key);
}

public String getValue(String key) {
    if (!hasKey(key)) return "";
    return (String)map.get(key);
}

}
