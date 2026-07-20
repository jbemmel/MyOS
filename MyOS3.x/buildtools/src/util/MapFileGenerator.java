package util;

import java.util.Arrays;

/**
 * A simple utility class that reads filenames from stdin, and prints out
 * a LD map file that maps all files within each directory into separate
 * .text sections
 * 
 * @author Jeroen
 *
 */
public final class MapFileGenerator {

	public static void main( String[] args ) {

		// First, sort the set of files on prefix, such that all files within 1 directory
		// are together
		Arrays.sort( args );
		String cur = null;
		for ( int i=0; i<args.length; ++i) {
			int slash = args[i].lastIndexOf('\\');
			String c = args[i].substring(0,slash).replaceAll("\\\\","_");
			String f = args[i].substring(slash+1).replaceAll(".cpp",".o");
			
			if (cur==null) {				// first component
				cur = c;
				System.out.println( cur + "_START = .;" );							
			} else if (!cur.equals(c)) {	// end old, start new
				System.out.println( cur + "_END = .;\n" );
				cur = c;
				System.out.println( cur + "_START = .;" );				
			} else {

			}
			System.out.println(  f + "(.text*)" );			
		}
		System.out.println( cur + "_END = .;\n" );			
	}
};