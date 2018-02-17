package linecompare;

import javafx.application.Application;
import javafx.scene.Scene;
import javafx.stage.Stage;

public class Main extends Application {

	private SourceViewer sourceViewer;

	@Override
	public void start(Stage primaryStage) throws Exception {
		//Parent root = FXMLLoader.load(getClass().getResource("sample.fxml"));
		primaryStage.setTitle("Select area");

		sourceViewer = new SourceViewer(primaryStage);
		primaryStage.setScene(new Scene(sourceViewer.mainPane, 1000, 800));

		primaryStage.show();
	}


	public static void main(String[] args) {
		launch(args);
	}
}
