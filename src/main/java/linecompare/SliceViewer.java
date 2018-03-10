package linecompare;

import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.DirectoryChooser;
import javafx.stage.Window;

import java.io.File;
import java.io.IOException;
import java.util.prefs.Preferences;

public class SliceViewer {
	Window window;
	SliceManager slices = new SliceManager();
	int selectedSliceIndex = 0;

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

		HBox buttonsPane = new HBox(10, toggleView, chooseButton);

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
				selectedSliceIndex = slices.images.size() - 1;
			}
			catch (IOException e) {
				new Alert(Alert.AlertType.ERROR, "Could not read directory", ButtonType.OK).show();
			}
		}
	}

	private void createSliceView(int index) {
		ImageView image = new ImageView(slices.images.get(index));


		int xIndex = slices.xIndices.get(index);

		Node view = null;

		if (xIndex < sliceBox.getChildren().size()) {
			view = sliceBox.getChildren().get(xIndex);
		}

		if (view != null && view instanceof VBox) {
			((VBox) view).getChildren().add(slices.yIndices.get(index), image);
		}
		else {
			sliceBox.getChildren().add(xIndex, new VBox(1, image));
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

		selectedSliceIndex = slices.images.size() - 1;
		createSliceView(selectedSliceIndex);
	}
}
