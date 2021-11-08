package edu.cs300;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Enumeration;
import java.util.Scanner;
import java.util.Vector;
import java.io.FileWriter;

class NewThread extends Thread {
	int reportId = -1;			//Updated when threads are created
	String specFileName;		//Name of each spec file
	String reportTitle;
	String searchString;
	String outFileName;
	Vector<Integer> begCol = new Vector<Integer>();
	Vector<Integer> endCol = new Vector<Integer>();
	Vector<String> colHeading = new Vector<String>();
	int numReports = 0;

	public void run() {
		System.out.println(specFileName);		//DELETE: confirms specID has been updated
		System.out.println(reportId);		//DELETE: confirms reportID has been updated
		//int numRecords = 0;	//DELETE
		try {
			File specFile = new File(specFileName);		//Opens each report spec file

			Scanner reportSpec = new Scanner(specFile);

			
			
			reportTitle = reportSpec.nextLine();
			searchString = reportSpec.nextLine();
			outFileName = reportSpec.nextLine();
			
			while(reportSpec.hasNext()) {
				reportSpec.useDelimiter("-");		//Changes delimiter to '-' in order to get beg col value

				begCol.add(reportSpec.nextInt());		//Adds beg col value to vector
				//System.out.println(begCol.get(numRecords));	//DELETE, just for Testing
				
				reportSpec.useDelimiter(",");		//Changes delimiter to '-' in order to get end col value

				endCol.add(reportSpec.skip("-").nextInt());		//Adds end col value to vector
				//System.out.println(endCol.get(numRecords));	//DELETE, just for Testing

				reportSpec.reset();					//Resets delimiter to default value

				colHeading.add(reportSpec.skip(",").nextLine());		//Gets remainder of line
				//System.out.println(colHeading.get(numRecords));	//DELETE, just for Testing

				//numRecords++;			//DELETE, just for Testing
			}
			

			reportSpec.close();

		} catch (FileNotFoundException ex) {
			System.out.println("FileNotFoundException triggered:"+ex.getMessage());
		}

		try {

			MessageJNI.writeReportRequest(reportId, numReports, searchString);

			File outFile = new File(outFileName);
			outFile.createNewFile();		//Create file for new report

			FileWriter outWriter = new FileWriter(outFileName);

			String recRecord = MessageJNI.readReportRecord(reportId);

				outWriter.write(reportTitle + "\t\n");
				for (int i = 0; i < colHeading.size(); i++) {
					outWriter.write(colHeading.get(i));
					if (i != colHeading.size() - 1) {
						outWriter.write("\t");
					}
					else{
						outWriter.write("\n");
					}
				}
			while (recRecord.length() > 1) {
				for (int i = 0; i < colHeading.size(); i++) {
					String recRecordSub = recRecord.substring(begCol.get(i) - 1, endCol.get(i));
					outWriter.write(recRecordSub);
					if (i != colHeading.size() - 1) {
						outWriter.write("\t");
					}
					else{
						outWriter.write("\n");
					}
				}
				recRecord = MessageJNI.readReportRecord(reportId);
				//System.out.println(reportId + ": HERE");

			}
			outWriter.close();

			//System.out.println(MessageJNI.readReportRecord(reportId));
		} catch (Exception e) {
			System.err.println("Error" + e);
		}
	}
}

public class ReportingSystem {


	public ReportingSystem() {
	  DebugLog.log("Starting Reporting System");
	}

	public int loadReportJobs() {
		int reportCounter = 0;
		try {
			
			File file = new File ("report_list.txt");

			Scanner reportList = new Scanner(file);

 		     //load specs and create threads for each report
			Vector<String> filenames = new Vector<String>();
			int numReps = 0;
			String whiteSpace;
  
			numReps = reportList.nextInt();
			System.out.println(numReps);		//DELETE: confirms number of reports in report_list.txt
			whiteSpace = reportList.nextLine();
			while (reportList.hasNextLine()) {
				filenames.add(reportList.nextLine());
			}
			System.out.println(filenames.get(0));

				DebugLog.log("Load specs and create threads for each report\nStart thread to request, process and print reports");
			for (int i = 0; i < numReps; i++) {		//creates a new thread for each report i < numReps
				NewThread reportThread = new NewThread();
				reportThread.reportId = i + 1;		//assigns correct index to ID
				reportThread.specFileName = filenames.get(i);
				reportThread.numReports = numReps;
				reportThread.start();
			}

			reportList.close();
		} catch (FileNotFoundException ex) {
			  System.out.println("FileNotFoundException triggered:"+ex.getMessage());
		}
		return reportCounter;

	}

	public static void main(String[] args) throws FileNotFoundException {


		   ReportingSystem reportSystem= new ReportingSystem();
		   int reportCount = reportSystem.loadReportJobs();


	}

}
