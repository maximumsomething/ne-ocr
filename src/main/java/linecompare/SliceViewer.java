package linecompare;

import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.stage.DirectoryChooser;
import javafx.stage.Window;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.prefs.Preferences;

public class SliceViewer {
	Window window;
	SliceManager slices = new SliceManager();
	ArrayList<StackPane> sliceViews = new ArrayList<>();

	// index in all slice-related arrays, such as slices.xIndices and yIndices, and sliceViews
	int selectedSliceIndex = 0;

	// usually the same as sliceViews.get(selectedSliceIndex)
	StackPane highlightedSliceView;

	GridPane mainPane = new GridPane();
	HBox sliceBox = new HBox(1);

	boolean addVertical = false;

	private Preferences prefs = Preferences.userRoot().node("SliceViewer");
	private static final String LOCATION_PREF = "slices location";

	SliceViewer(Window window) {
		this.window = window;
		setupLayout();

		String prevFile = prefs.get(LOCATION_PREF, "");
		if (!prevFile.equals("")) loadDir(new File(prevFile));
	}

	private void setupLayout() {
		mainPane.setAlignment(Pos.TOP_CENTER);
		mainPane.setHgap(10);
		mainPane.setVgap(10);

		Button chooseButton = new Button("Choose Save Location");
		chooseButton.setOnAction(event -> showChooser());

		Button deleteButton = new Button("Delete");
		deleteButton.setOnAction(event -> {
			slices.images.remove(selectedSliceIndex);
			slices.xIndices.remove(selectedSliceIndex);
			slices.yIndices.remove(selectedSliceIndex);

			//highlightedSliceView.parent

			sliceViews.remove(selectedSliceIndex);

			updateSelectedSlice(selectedSliceIndex);
		});

		ToggleGroup hvToggle = new ToggleGroup();
		ToggleButton toggleHorizontal = new ToggleButton("Horizontal");
		toggleHorizontal.setToggleGroup(hvToggle);
		ToggleButton toggleVertical = new ToggleButton("Vertical");
		toggleVertical.setToggleGroup(hvToggle);

		hvToggle.selectedToggleProperty().addListener((observable, oldValue, newValue) -> {
			if (newValue == toggleVertical) addVertical = true;
			else addVertical = false;
		});
		HBox toggleView = new HBox(0, toggleHorizontal, toggleVertical);

		HBox buttonsPane = new HBox(10, deleteButton, toggleView, chooseButton);

		mainPane.add(buttonsPane, 0, 0);
		mainPane.add(new ScrollPane(sliceBox), 0, 1);
	}

	private void showChooser() {
		DirectoryChooser chooser = new DirectoryChooser();
		chooser.setTitle("Select folder to save image images");
		File dir = chooser.showDialog(window);
		loadDir(dir);

		try { prefs.put(LOCATION_PREF, dir.getCanonicalPath()); } catch (IOException e) {}
	}
	private void loadDir(File dir) {
		if (dir != null && dir.isDirectory()) {
			slices.sliceDirectory = dir;
			try {
				slices.loadSlices();
				for (int i = 0; i != slices.images.size(); ++i) {
					createSliceView(i);
				}
				updateSelectedSlice(slices.images.size() - 1);
			}
			catch (IOException e) {
				new Alert(Alert.AlertType.ERROR, "Could not read directory", ButtonType.OK).show();
			}
		}
	}

	private void updateSelectedSlice(int index) {
		if (highlightedSliceView != null) {
			highlightedSliceView.setStyle("");
		}

		if (index >= 0 && index < sliceViews.size()) {
			highlightedSliceView = sliceViews.get(index);
			highlightedSliceView.setStyle("-fx-border-color: blue; -fx-border-width: 2; -fx-border-style:solid");
		}

		selectedSliceIndex = index;
	}

	private void createSliceView(final int index) {
		StackPane imageView = new StackPane(new ImageView(slices.images.get(index)));
		sliceViews.add(imageView);

		imageView.addEventFilter(MouseEvent.MOUSE_CLICKED, event -> {
			updateSelectedSlice(index);
		});

		int xIndex = slices.xIndices.get(index);

		Node sliceColumn = null;

		if (xIndex < sliceBox.getChildren().size()) {
			sliceColumn = sliceBox.getChildren().get(xIndex);
		}

		if (sliceColumn != null && sliceColumn instanceof VBox) {
			((VBox) sliceColumn).getChildren().add(slices.yIndices.get(index), imageView);
		}
		else {
			sliceBox.getChildren().add(xIndex, new VBox(1, imageView));
		}
	}

	public void addSliceImage(Image image) {
		int x, y;
		if (selectedSliceIndex >= 0) {
			int selX = slices.xIndices.get(selectedSliceIndex);
			int selY = slices.yIndices.get(selectedSliceIndex);

			if (addVertical) {
				x = selX;
				y = selY + 1;
			}
			else {
				x = selX + 1;
				y = selY;
			}
		}
		else {
			// first slice added
			x = 0;
			y = 0;
		}

		slices.images.add(image);
		slices.xIndices.add(x);
		slices.yIndices.add(y);

		int index = slices.images.size() - 1;
		createSliceView(index);
		updateSelectedSlice(index);
	}
}
