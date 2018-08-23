package linecompare;

import javafx.application.Platform;
import javafx.geometry.Pos;
import javafx.scene.control.Label;
import javafx.scene.control.ScrollPane;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.scene.text.Font;
import javafx.stage.Window;

import java.io.File;

public class ResultsViewer {
	Window window;
	GridPane mainPane = new GridPane();
	VBox resultsPane = new VBox(5);
	HBox selectionViewer = new HBox(3);
	String characterFolder = "Working Files/Extracted Characters";
	Label workingLabel = new Label();
	MALCaller programCaller = new MALCaller();

	ResultsViewer(Window window) {
		this.window = window;

		mainPane.setAlignment(Pos.TOP_CENTER);

		resultsPane.setFillWidth(true);
		resultsPane.setAlignment(Pos.TOP_CENTER);

		ScrollPane resultsScroll = new ScrollPane(resultsPane);
		resultsScroll.setPrefViewportWidth(10000000000.0);

		mainPane.add(resultsScroll, 0, 1);

		workingLabel.setText("Working...");
		workingLabel.setFont(Font.font(14));
	}

	private static final int selectionViewImageWidth = 100;
	void addToSelectionView(Image img) {
		ImageView view = new ImageView(img);
		view.setPreserveRatio(true);
		view.setFitWidth(selectionViewImageWidth);
		selectionViewer.getChildren().add(view);
	}

	void useImage(Image character) {

		if (!programCaller.running && !mainPane.getChildren().contains(workingLabel)) {
			mainPane.add(workingLabel, 0, 0);
		}

		selectionViewer.getChildren().clear();
		addToSelectionView(character);
		if (!resultsPane.getChildren().contains(selectionViewer)) {
			resultsPane.getChildren().add(0, selectionViewer);
		}

		programCaller.findChar(character, new MALCaller.Callback() {
			@Override
			public void bwImage(Image bwimage) {
				Platform.runLater(() -> {
					addToSelectionView(bwimage);
				});
			}

			@Override
			public void finished(String charNames) {
				String[] nameArray = charNames.split("\n");
				Platform.runLater(() -> ResultsViewer.this.gotResults(nameArray));
			}
		});
	}

	void gotResults(String[] results) {

		resultsPane.getChildren().clear();
		resultsPane.getChildren().add(selectionViewer);
		mainPane.getChildren().remove(workingLabel);

		for (int i = 0; i < results.length; ++i) {

			Image image = new Image((new File(
					Main.programDir.getAbsolutePath() + File.separator + characterFolder +
							File.separator + results[i])).toURI().toString());

			ImageView imgView = new ImageView(image);
			imgView.setPreserveRatio(true);

			String pageNum = results[i].substring(0, results[i].indexOf(" - "));

			Label label = new Label();
			label.setText(pageNum);

			VBox view = new VBox();
			view.getChildren().add(imgView);
			view.getChildren().add(label);

			resultsPane.getChildren().add(view);
		}
	}
}
