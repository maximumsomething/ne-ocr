package linecompare;

import javafx.application.Platform;
import javafx.geometry.Pos;
import javafx.scene.Cursor;
import javafx.scene.Node;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.image.*;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.scene.text.Text;
import javafx.scene.text.TextFlow;
import javafx.stage.FileChooser;
import javafx.stage.Window;

import java.awt.*;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DecimalFormat;
import java.util.prefs.Preferences;

public class ResultsViewer {
	private Window window;
	public GridPane mainPane = new GridPane();
	private VBox resultsPane = new VBox(5);
	private HBox selectionViewer = new HBox(3);
	private Button stopButton = new Button("Stop");

	private ToggleButton skeletonSettingsButton;
	Slider dotSizeSlider, holeSizeSlider;
	double savedDotSize = 3, savedHoleSize = 1.5;

	ToggleButton drawButton, eraserButton;
	Button undoButton, redoButton, clearButton;
	DrawingView drawingView = new DrawingView();

	Image character, skeleton;

	private String characterFolder = "Extracted Characters";

	ExternalCaller programCaller = new ExternalCaller();

	private String dictionaryPDF;
	private Preferences prefs = Preferences.userRoot().node("ResultsViewer");
	private String DICTIONARY_PDF_PREF = "Dictionary PDF to open";

	ResultsViewer(Window window) {
		this.window = window;

		mainPane.setAlignment(Pos.TOP_CENTER);

		resultsPane.setFillWidth(true);
		resultsPane.setAlignment(Pos.TOP_CENTER);

		ScrollPane resultsScroll = new ScrollPane(resultsPane);
		resultsScroll.setPrefViewportWidth(10000000000.0);

		mainPane.setVgap(10);
		mainPane.add(selectionViewer, 0, 2);
		mainPane.add(resultsScroll, 0, 3);

		/*Label workingLabel = new Label();
		workingLabel.setText("Working...");
		workingLabel.setFont(Font.font(14));
		topPane.setLeft(workingLabel);*/

		stopButton.setOnAction(event -> {
			programCaller.stop();
			hideStopButton();
		});
		stopButton.setVisible(false);

		dotSizeSlider = new Slider(0, 5, 3);
		HBox dotSizeSliderBox = setupSkeletonSlider(dotSizeSlider);
		Label dotSliderLabel = new Label("Dot Size");
		holeSizeSlider = new Slider(0, 5, 1.5);
		HBox holeSizeSliderBox = setupSkeletonSlider(holeSizeSlider);
		Label holeSliderLabel = new Label("Hole size");

		VBox sliderPane = new VBox(dotSliderLabel, dotSizeSliderBox, holeSliderLabel, holeSizeSliderBox);

		skeletonSettingsButton = new ToggleButton("⚙︎");
		skeletonSettingsButton.setOnAction(event -> {
			if (skeletonSettingsButton.isSelected()) mainPane.add(sliderPane, 0, 1);
			else mainPane.getChildren().remove(sliderPane);
		});

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
		BorderPane topPane = new BorderPane();
		topPane.setRight(new HBox(stopButton, skeletonSettingsButton));
		topPane.setLeft(drawingControlsPane);

		mainPane.add(topPane, 0, 0);
	}

	private HBox setupSkeletonSlider(Slider slider) {
		slider.setShowTickLabels(true);
		slider.setBlockIncrement(0.5);
		Label valueLabel = new Label(Double.toString(slider.getValue()) + "%");
		slider.valueProperty().addListener((observable, oldValue, newValue) -> {
			if (slider.isDisabled()) return;
			valueLabel.setText(new DecimalFormat("#.##").format(newValue) + "%");
			skelSettingsSliderChanged();
		});
		slider.setPrefWidth(10000);
		valueLabel.setMinWidth(50);
		return new HBox(slider, valueLabel);
	}

	private void skelSettingsSliderChanged() {
		programCaller.changeSkelParams(dotSizeSlider.getValue()/100, holeSizeSlider.getValue()/100);
		resultsPane.getChildren().clear();
		showStopButton();
	}

	private void disableSkelSettingsSliders() {
		if (!dotSizeSlider.isDisabled()) {
			savedDotSize = dotSizeSlider.getValue();
			savedHoleSize = holeSizeSlider.getValue();
			dotSizeSlider.setDisable(true);
			dotSizeSlider.setValue(0);
			holeSizeSlider.setDisable(true);
			holeSizeSlider.setValue(0);
		}
	}
	private void enableSkelSettingsSliders() {
		if (dotSizeSlider.isDisabled()) {
			dotSizeSlider.setValue(savedDotSize);
			dotSizeSlider.setDisable(false);
			holeSizeSlider.setValue(savedHoleSize);
			holeSizeSlider.setDisable(false);
		}
	}

	private void activateDrawing() {
		selectionViewer.getChildren().clear();
		selectionViewer.getChildren().add(drawingView);
		drawingView.isActive = true;

		undoButton.setDisable(false);
		redoButton.setDisable(false);
		clearButton.setDisable(false);
		eraserButton.setDisable(false);

		disableSkelSettingsSliders();
	}
	public void deactivateDrawing() {
		drawingView.isActive = false;
		drawButton.setSelected(false);

		undoButton.setDisable(true);
		redoButton.setDisable(true);
		clearButton.setDisable(true);
		eraserButton.setDisable(true);

		enableSkelSettingsSliders();
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

		showStopButton();

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

	boolean isPDF(String filename) {
		int i = filename.lastIndexOf('.');
		String ext = i > 0 ? filename.substring(i + 1) : "";
		return ext.toUpperCase().equals("PDF");
	}

	void openPDFToPage(int pageNum) {
		if (dictionaryPDF == null) dictionaryPDF = prefs.get(DICTIONARY_PDF_PREF, null);
		File dictionaryPDFFile;
		if (dictionaryPDF == null || !(dictionaryPDFFile = new File(dictionaryPDF)).isFile()) {

			FileChooser chooser = new FileChooser();
			chooser.setTitle("Select dictionary PDF to open");
			chooser.getExtensionFilters().setAll(new FileChooser.ExtensionFilter("PDF", "*.pdf", "*.PDF"));

			dictionaryPDFFile = chooser.showOpenDialog(window);
			if (dictionaryPDFFile == null || !dictionaryPDFFile.isFile() || !isPDF(dictionaryPDFFile.getName())) return;
			dictionaryPDF = dictionaryPDFFile.getAbsolutePath();

			prefs.put(DICTIONARY_PDF_PREF, dictionaryPDF);
		}

		// Desktop.browse() is broken for file:/// URLs that contain anchors (at least on MacOS)
		String tempHTML = "<meta http-equiv=\"refresh\" content=\"0; URL='"
				+ dictionaryPDFFile.toURI().toString()
				+ "#page="+pageNum+"'\" />";
		try {
			File tempFile = File.createTempFile("redirect",".html");
			FileWriter tempFileWriter = new FileWriter(tempFile);
			tempFileWriter.write(tempHTML);
			tempFileWriter.close();

			Desktop.getDesktop().open(tempFile);
		}
		//catch (URISyntaxException e) { throw new RuntimeException(e); }
		catch (IOException e) {
			new Alert(Alert.AlertType.ERROR, "Could not open PDF", ButtonType.OK);
		}
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
			int pageNum;
			try {
				pageNum = Integer.parseInt(thisResult.substring(thisResult.indexOf("Page ") + 5, thisResult.indexOf(" - ")));

				/*//Page 178 is duplicated in the PDF
				int printOffset;
				if (pageNum <= 216) printOffset = 36;
				else printOffset = 37;*
				 */
				int printOffset = 36;
				pageNumStr = String.format("Page %d (PDF), %d (Print)", pageNum, pageNum - printOffset);
			}
			catch (StringIndexOutOfBoundsException e) {
				System.out.println("invalid result: " + results[i]);
				continue;
			}

			Label pageLabel = new Label();
			pageLabel.setText(pageNumStr);

			Node below = pageLabel;

			if (thisResult.startsWith("Alternates")) {
				String refStr = thisResult.substring(thisResult.indexOf("Page "), thisResult.indexOf(" - ") + 3) + thisResult.substring(thisResult.indexOf("ref"));
				Image refImg = new Image(new File(Main.programDir.getAbsolutePath() + File.separator + characterFolder +
						File.separator + "Alternates Refs" + File.separator + refStr).toURI().toString());
				ImageView refImgView = new ImageView(refImg);

				refImgView.setPreserveRatio(true);
				refImgView.setFitHeight(25);

				TextFlow alternateLabel = new TextFlow(new Text("Alternate "), refImgView);

				below = new VBox(alternateLabel, below);
			}



			if (false) { // If showing database add button
				Button confirmMatch = new Button("✓");
				confirmMatch.setTooltip(confirmMatchTooltip);

				confirmMatch.setOnAction(event -> {
					programCaller.addMatch(thisResult, character, skeleton);
				});

				BorderPane belowPane = new BorderPane();
				belowPane.setLeft(below);
				belowPane.setRight(confirmMatch);
				below = belowPane;
			}

			VBox resultView = new VBox();
			resultView.getChildren().add(imgView);
			resultView.getChildren().add(below);

			resultView.setOnMouseClicked(event -> openPDFToPage(pageNum));
			resultView.setCursor(Cursor.HAND);

			resultsPane.getChildren().add(resultView);
		}

		// Needed due to bug in JavaFX. If this is not done, the resultsPane would be
		// smaller than its contents until it is clicked.
		Platform.runLater(() -> resultsPane.setPrefHeight(resultsPane.getHeight()));
	}
}
