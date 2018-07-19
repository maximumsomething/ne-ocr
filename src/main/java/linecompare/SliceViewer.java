package linecompare;

import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.ToggleButton;
import javafx.scene.control.ToggleGroup;
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
	ArrayList<ArrayList<Slice>> slices = new ArrayList<>();

	Slice selectedSlice = null;

	GridPane mainPane = new GridPane();
	HBox sliceBox = new HBox(1);

	boolean addVertical = false;
	boolean addAbove = false;

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
		/*
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
		}*/
	}

	private void updateSelectedSlice(Slice slice) {
		if (selectedSlice != null) {
			selectedSlice.wrapperView.setStyle("");
		}

		slice.wrapperView.setStyle("-fx-border-color: blue; -fx-border-width: 2; -fx-border-style:solid");

		selectedSlice = slice;
	}

	public void addSliceImage(Image image) {
		StackPane wrapperView = new StackPane(new ImageView(image));

		final Slice slice = new Slice(image, wrapperView);

		wrapperView.addEventFilter(MouseEvent.MOUSE_CLICKED, event -> {
			updateSelectedSlice(slice);
		});

		if (selectedSlice != null && slices.size() != 0) {
			if (addVertical) {
				int selectedY = selectedSlice.column.indexOf(selectedSlice);
				if (addAbove) selectedSlice.column.add(selectedY, slice);
				else selectedSlice.column.add(selectedY + 1, slice);

				selectedSlice.columnView.getChildren().add(wrapperView);

				slice.column = selectedSlice.column;
				slice.columnView = selectedSlice.columnView;
			}
			else {
				int selectedX = slices.indexOf(selectedSlice.column);
				createSliceColumn(slice, selectedX + 1);
			}
		}
		else {
			createSliceColumn(slice, 0);
		}
		updateSelectedSlice(slice);
	}
	private void createSliceColumn(Slice slice, int index) {
		ArrayList<Slice> newColumn = new ArrayList<>(1);
		newColumn.add(slice);
		slice.column = newColumn;
		slices.add(index, newColumn);

		VBox columnView = new VBox(1, slice.wrapperView);
		slice.columnView = columnView;
		sliceBox.getChildren().add(index, columnView);
	}
}






