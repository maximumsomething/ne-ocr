package linecompare;

import javafx.application.Platform;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.image.*;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.Window;

import java.io.File;

public class ResultsViewer {
	private Window window;
	GridPane mainPane = new GridPane();
	private VBox resultsPane = new VBox(5);
	private HBox selectionViewer = new HBox(3);
	Button stopButton = new Button("Stop");
	ToggleButton drawButton, eraserButton;
	Button undoButton, redoButton, clearButton;
	DrawingView drawingView = new DrawingView();

	Image character, skeleton;

	private String characterFolder = "Extracted Characters";

	ExternalCaller programCaller = new ExternalCaller();

	ResultsViewer(Window window) {
		this.window = window;

		mainPane.setAlignment(Pos.TOP_CENTER);

		resultsPane.setFillWidth(true);
		resultsPane.setAlignment(Pos.TOP_CENTER);

		ScrollPane resultsScroll = new ScrollPane(resultsPane);
		resultsScroll.setPrefViewportWidth(10000000000.0);

		mainPane.setVgap(10);
		mainPane.add(selectionViewer, 0, 1);
		mainPane.add(resultsScroll, 0, 2);

		/*Label workingLabel = new Label();
		workingLabel.setText("Working...");
		workingLabel.setFont(Font.font(14));
		topPane.setLeft(workingLabel);*/

		BorderPane topPane = new BorderPane();

		stopButton.setOnAction(event -> {
			programCaller.stop();
			mainPane.getChildren().remove(topPane);
		});
		topPane.setRight(stopButton);
		stopButton.setVisible(false);

		drawButton = new ToggleButton("✎");
		drawButton.setTooltip(new Tooltip("Draw character"));
		eraserButton = new ToggleButton("⌫");
		eraserButton.setTooltip(new Tooltip("Eraser tool"));
		undoButton = new Button("⃔");
		undoButton.setTooltip(new Tooltip("Undo"));
		redoButton = new Button("⃕");
		redoButton.setTooltip(new Tooltip("Redo"));
		clearButton = new Button("Clear");

		drawButton.setOnAction(event -> {
			if (drawButton.isSelected()) activateDrawing();
			else deactivateDrawing();
		});
		undoButton.setOnAction(event -> {
			drawingView.undo();
		});
		redoButton.setOnAction(event -> {
			drawingView.redo();
		});
		clearButton.setOnAction(event -> {
			drawingView.clear();
		});
		drawingView.onStrokeFinished = img -> {
			useImage(img);
		};

		GridPane drawingControlsPane = new GridPane();
		drawingControlsPane.add(drawButton, 0, 0);
		drawingControlsPane.add(eraserButton, 1, 0);
		drawingControlsPane.add(undoButton, 2, 0);
		drawingControlsPane.add(redoButton, 3, 0);
		drawingControlsPane.add(clearButton, 4, 0);

		deactivateDrawing();

		topPane.setLeft(drawingControlsPane);

		mainPane.add(topPane, 0, 0);
	}

	private void activateDrawing() {
		selectionViewer.getChildren().clear();
		selectionViewer.getChildren().add(drawingView);
		drawingView.isActive = true;

		undoButton.setDisable(false);
		redoButton.setDisable(false);
		clearButton.setDisable(false);
		eraserButton.setDisable(false);
	}
	public void deactivateDrawing() {
		drawingView.isActive = false;
		drawButton.setSelected(false);

		undoButton.setDisable(true);
		redoButton.setDisable(true);
		clearButton.setDisable(true);
		eraserButton.setDisable(true);
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

	void hideStopButton() {
		stopButton.setVisible(false);
	}
	void showStopButton() {
		stopButton.setVisible(true);
	}

	public void useImageFromDoc(Image character) {
		deactivateDrawing();
		selectionViewer.getChildren().clear();
		addToSelectionView(character);
		useImage(character);
	}

	private void useImage(Image character) {
		this.character = character;

		resultsPane.getChildren().clear();

		if (!programCaller.running) {
			showStopButton();
		}

		programCaller.findChar(character, new ExternalCaller.Callback() {
			@Override
			public void bwImage(Image bwimage) {
				ResultsViewer.this.skeleton = bwimage;
				Platform.runLater(() -> {
					if (selectionViewer.getChildren().size() > 1) selectionViewer.getChildren().remove(1);
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
				hideStopButton();
				System.out.println("failed to find results");
			}
		});
	}

	void gotResults(String[] results) {

		hideStopButton();

		Tooltip confirmMatchTooltip = new Tooltip("Confirm match and add to database");

		for (int i = 0; i < results.length; ++i) {

			Image image = new Image((new File(
					Main.programDir.getAbsolutePath() + File.separator + characterFolder +
							File.separator + results[i])).toURI().toString());

			ImageView imgView = new ImageView(image);
			imgView.setPreserveRatio(true);
			if (image.getWidth() > resultsPane.getWidth()) imgView.fitWidthProperty().bind(resultsPane.widthProperty());

			String thisResult = results[i];
			String pageNumStr;
			try {
				//pageNumStr = thisResult.substring(0, results[i].indexOf(" - "));
				int pageNum = Integer.parseInt(thisResult.substring(5, thisResult.indexOf(" - ")));

				// Page 178 is duplicated in the PDF
				int printOffset;
				if (pageNum <= 216) printOffset = 36;
				else printOffset = 37;
				pageNumStr = String.format("Page %d (PDF), %d (Print)", pageNum, pageNum - printOffset);
			}
			catch (StringIndexOutOfBoundsException e) {
				System.out.println("invalid result: " + results[i]);
				continue;
			}

			Label label = new Label();
			label.setText(pageNumStr);


			Node below;
			if (false) { // If showing database add button
				Button confirmMatch = new Button("✓");
				confirmMatch.setTooltip(confirmMatchTooltip);

				confirmMatch.setOnAction(event -> {
					programCaller.addMatch(thisResult, character, skeleton);
				});

				BorderPane belowPane = new BorderPane();
				belowPane.setLeft(label);
				belowPane.setRight(confirmMatch);
				below = belowPane;
			}
			else {
				below = label;
			}

			VBox view = new VBox();
			view.getChildren().add(imgView);
			view.getChildren().add(below);

			resultsPane.getChildren().add(view);
		}
	}
}
