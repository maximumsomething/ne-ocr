package linecompare;

import javafx.application.Platform;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ScrollPane;
import javafx.scene.image.*;
import javafx.scene.layout.BorderPane;
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
	BorderPane workingPane = new BorderPane();

	MALCaller programCaller = new MALCaller();

	ResultsViewer(Window window) {
		this.window = window;

		mainPane.setAlignment(Pos.TOP_CENTER);

		resultsPane.setFillWidth(true);
		resultsPane.setAlignment(Pos.TOP_CENTER);

		ScrollPane resultsScroll = new ScrollPane(resultsPane);
		resultsScroll.setPrefViewportWidth(10000000000.0);

		mainPane.add(selectionViewer, 0, 1);
		mainPane.add(resultsScroll, 0, 2);

		Label workingLabel = new Label();
		workingLabel.setText("Working...");
		workingLabel.setFont(Font.font(14));
		workingPane.setLeft(workingLabel);

		Button stopButton = new Button("Stop");
		stopButton.setOnAction(new EventHandler<ActionEvent>() {
			@Override
			public void handle(ActionEvent event) {
				programCaller.stop();
				mainPane.getChildren().remove(workingPane);
			}
		});
		workingPane.setRight(stopButton);
	}

	private static final int selectionViewImageWidth = 150;
	void addToSelectionView(Image img) {

		ImageView view = new ImageView(resampleImage(img, (int) Math.max(1, Math.round(selectionViewImageWidth / img.getWidth()))));
		view.setSmooth(false);
		view.setPreserveRatio(true);
		view.setFitWidth(selectionViewImageWidth);
		selectionViewer.getChildren().add(view);
	}

	// from StackOverflow
	private Image resampleImage(Image input, int scaleFactor) {
		final int W = (int) input.getWidth();
		final int H = (int) input.getHeight();
		final int S = scaleFactor;

		WritableImage output = new WritableImage(
				W * S,
				H * S
		);

		PixelReader reader = input.getPixelReader();
		PixelWriter writer = output.getPixelWriter();

		for (int y = 0; y < H; y++) {
			for (int x = 0; x < W; x++) {
				final int argb = reader.getArgb(x, y);
				for (int dy = 0; dy < S; dy++) {
					for (int dx = 0; dx < S; dx++) {
						writer.setArgb(x * S + dx, y * S + dy, argb);
					}
				}
			}
		}

		return output;
	}

	void useImage(Image character) {
		resultsPane.getChildren().clear();

		if (!programCaller.running && !mainPane.getChildren().contains(workingPane)) {
			mainPane.add(workingPane, 0, 0);
		}

		selectionViewer.getChildren().clear();
		addToSelectionView(character);


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
			@Override
			public void failed() {
				mainPane.getChildren().remove(workingPane);
			}
		});
	}

	void gotResults(String[] results) {

		mainPane.getChildren().remove(workingPane);

		for (int i = 0; i < results.length; ++i) {

			Image image = new Image((new File(
					Main.programDir.getAbsolutePath() + File.separator + characterFolder +
							File.separator + results[i])).toURI().toString());

			ImageView imgView = new ImageView(image);
			imgView.setPreserveRatio(true);
			if (image.getWidth() > resultsPane.getWidth()) imgView.fitWidthProperty().bind(resultsPane.widthProperty());

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
