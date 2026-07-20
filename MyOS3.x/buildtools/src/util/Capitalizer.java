package util;

/**
* This class provides an XSLT extension function that
* may be utilized by Xalan-Java extension mechanism.
*/
public class Capitalizer {

    /**
     * This method capitalizes the first character
     * in the provided string.
     * @return modified string
     */
    public static String capitalize(String str) {
        return Character.toUpperCase(str.charAt(0)) + str.substring(1);
    }

    /**
     * This method uncapitalizes the first character in the provided string.
     * @return modified string
     */
    public static String uncapitalize(String str) {
        return Character.toLowerCase(str.charAt(0)) + str.substring(1);
    }

    /**
     * This method capitalizes the whole string
     * @return modified string
     */
    public static String toUpperCase(String str) {
        return str.toUpperCase();
    }

    /**
     * This method makes the whole string lower case
     * @return modified string
     */
    public static String toLowerCase(String str) {
        return str.toLowerCase();
    }    
    
    /**
     * Calculates indexOf(string,substring) counting '\' characters
     * @author Jeroen
     *
     */
	public static int indexOf( String full, String sub ) {
		int r = full.indexOf( sub );
		int slashes = 0; 
		for (int i=0; i<r; ++i) {			
			if (full.charAt(i)=='\\') ++slashes;
		}
		return r - slashes;
	}
	
	/**
	 * Counts the number of chars 'c' in string s
	 * Made param 'String' to simplify xsl usage (cannot use ' quotes)
	 */
	public static int countChars( String s, String cs ) {
		int count = 0;
		int i = -1;
		char c = cs.charAt(0);		
		while (( i = s.indexOf(c,i+1) ) >= 0) ++count;
		return count;		
	}
}

