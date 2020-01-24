package linecompare;

import javafx.application.Application;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.File;
import java.net.URISyntaxException;

public class Main extends Application {

	public static SourceViewer sourceViewer;
	public static SliceViewer sliceViewer;
	public static ResultsViewer resultsViewer;

	// Contains connection_compare executable and the characters from the dictionary
	public static File programDir;

	//public static String matlabRoot = "/Applications/MATLAB_R2018a.app/";


	@Override
	public void start(Stage primaryStage) throws Exception {


		PosRestoreStage sourceStage = new PosRestoreStage();
		sourceStage.setTitle("Select character");
		sourceViewer = new SourceViewer(sourceStage);
		sourceStage.setScene(new Scene(sourceViewer.mainPane, 700, 800));
		sourceStage.restoreSize("source");
		sourceStage.show();

		PosRestoreStage resultsStage = new PosRestoreStage();
		resultsStage.setTitle("Results");
		resultsViewer = new ResultsViewer(resultsStage);
		resultsStage.setScene(new Scene(resultsViewer.mainPane, 300, 800));
		resultsStage.restoreSize("results");
		resultsStage.show();

		/*
		PosRestoreStage sliceStage = new PosRestoreStage();
		sliceStage.setTitle("Book slices");
		sliceViewer = new SliceViewer(sliceStage);
		sliceStage.setScene(new Scene(sliceViewer.mainPane, 700, 800));
		sliceStage.restoreSize("slices");
		sliceStage.show();*/
	}

	private static File findJar() {
		File jarDir;
		try {
			jarDir = new File(Main.class.getProtectionDomain().getCodeSource().getLocation()
					.toURI()).getParentFile();
		}
		catch (URISyntaxException e) {
			throw new RuntimeException(e);
		}

		if (new File(jarDir, "Extracted Characters").isDirectory()
				&& new File(jarDir, "connection_compare").isFile()) {
			return jarDir;
		}
		return null;
	}

	public static void main(String[] args) {
		if (args.length > 0) {
			programDir = new File(args[0]);
		}
		else {
			programDir = findJar();
		}
		if (programDir == null) {
			System.out.println("Invalid program directory. Run the .jar from the program directory or pass it as an argument.");
			System.exit(1);
		}

		launch(args);
	}
}
