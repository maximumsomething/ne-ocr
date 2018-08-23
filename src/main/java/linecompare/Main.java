package linecompare;

import javafx.application.Application;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.File;

public class Main extends Application {

	public static SourceViewer sourceViewer;
	public static SliceViewer sliceViewer;
	public static ResultsViewer resultsViewer;

	public static File programDir = new File("/Users/max/Documents/MATLAB");

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


	public static void main(String[] args) {
		launch(args);
	}
}
