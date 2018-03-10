package linecompare;

import javafx.application.Application;
import javafx.scene.Scene;
import javafx.stage.Stage;

public class Main extends Application {

	public static SourceViewer sourceViewer;
	public static SliceViewer sliceViewer;

	@Override
	public void start(Stage primaryStage) throws Exception {

		PosRestoreStage sourceStage = new PosRestoreStage();
		sourceStage.setTitle("Select area");
		sourceViewer = new SourceViewer(sourceStage);
		sourceStage.setScene(new Scene(sourceViewer.mainPane, 700, 800));
		sourceStage.restoreSize("source");
		sourceStage.show();

		PosRestoreStage sliceStage = new PosRestoreStage();
		sliceStage.setTitle("Book slices");
		sliceViewer = new SliceViewer(sliceStage);
		sliceStage.setScene(new Scene(sliceViewer.mainPane, 700, 800));
		sliceStage.restoreSize("slices");
		sliceStage.show();
	}


	public static void main(String[] args) {
		launch(args);
	}
}
