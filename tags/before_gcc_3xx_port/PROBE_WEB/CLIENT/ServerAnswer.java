import java.util.*;

public class ServerAnswer
{
    private HashMap map;
    private String  error_message;
    private boolean is_server_problem = false;

    private String removeLF(String str) {
        int lf = str.indexOf("\n");
        if (lf == -1) return str;
        if (lf == 0) return removeLF(str.substring(1));
        return str.substring(0, lf) + removeLF(str.substring(lf+1));
    }
    
    private String removeTags(String str) {
        String  res   = "";
        int     start = 0;
        boolean done  = false;

        while (!done) {
            int open  = str.indexOf('<', start);
            int close = -1;

            if (open != -1) {
                close = str.indexOf('>', open+1);
            }

            if (close == -1) {
                if (!res.equals("")) res += " ";
                res  += removeLF(str.substring(start));
                done  = true;
            }
            else {
                if (start != open) {
                    if (!res.equals("")) res += " ";
                    res += removeLF(str.substring(start, open));
                }
                start = close+1;
            }
        }

        return res;
    }

    public ServerAnswer(String answer, boolean needResult, boolean beNoisy) throws Exception {
        map                  = new HashMap();
        error_message        = null;
        boolean format_error = false;

        String parseAnswer = new String(answer)+"\n";

        int start = 0;
        int nl    = parseAnswer.indexOf('\n');

        while (nl != -1) {
            String line = parseAnswer.substring(start, nl);
            if (!line.equals("")) {
                int    eq   = line.indexOf('=');
                if (eq !=  -1) {
                    String key   = line.substring(0, eq);
                    String value = line.substring(eq+1);
                    map.put(key, value);
                    if (beNoisy) System.out.println("    ANSWER: key='"+key+"' value='"+value+"'");
                }
                else {
                    format_error = true;
                }
            }

            start = nl+1;
            nl    = parseAnswer.indexOf('\n', start);
        }

        if (format_error) {
            if (hasKey("result") && !getValue("result").equals("ok")) {
                if (hasKey("message")) {
                    error_message = getValue("message");
                }
                else {
                    // should not occur
                    error_message     = "Result '"+getValue("result")+"' without message";
                    is_server_problem = true;
                }
            }
            else {
                error_message     = removeTags(parseAnswer);
                is_server_problem = true;
            }
        }
        else {
            if (needResult) {
                if (hasKey("result")) {
                    String result = getValue("result");
                    if (!result.equals("ok")) {
                        if (hasKey("message")) {
                            error_message = getValue("message");
                        }
                        else {
                            // should not occur
                            error_message     = "Result '"+getValue("result")+"' without message";
                            is_server_problem = true;
                        }
                    }
                }
                else {
                    error_message     = "no 'result' found";
                    is_server_problem = true;
                }
            }
        }
    }

    public boolean hasKey(String key) { return !hasError() && map.containsKey(key); }

    public String getValue(String key) throws Exception {
        if (!hasKey(key)) {
            Toolkit.AbortWithError("Unknown key '"+key+"' requested");
        }
        return (String)map.get(key);
    }

    public int getIntValue(String key) throws Exception {
        return Integer.parseInt(getValue(key));
    }

    // error handling

    public boolean hasError() { return error_message != null; }

    public String getError() {
        if (!hasError()) return "no error";
        return error_message;
    }

    public void handleError() throws Exception {
        if (hasError()) {
            if (is_server_problem) {
                Toolkit.AbortWithServerProblem("[ServerAnswer] "+getError());
            }
            else {
                Toolkit.AbortWithError("[ServerAnswer] "+getError());
            }
        }
    }

}
