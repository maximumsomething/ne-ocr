package linecompare;

import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.Alert;
import javafx.scene.control.Button;
import javafx.scene.control.ButtonType;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.DirectoryChooser;
import javafx.stage.Window;

import java.io.File;
import java.io.IOException;

public class SliceViewer {
	Window window;
	SliceManager slices = new SliceManager();
	int selectedSliceIndex = 0;

	GridPane mainPane = new GridPane();
	HBox sliceBox = new HBox(1);

	SliceViewer(Window window) {
		this.window = window;
		setupLayout();
	}

	private void setupLayout() {
		mainPane.setAlignment(Pos.TOP_CENTER);
		mainPane.setHgap(10);
		mainPane.setVgap(10);
		mainPane.setPadding(new Insets(25, 25, 25, 25));

		Button chooseButton = new Button("Choose Save Location");
		chooseButton.setOnAction(new EventHandler<ActionEvent>() {
			public void handle(ActionEvent event) {

			}
		});
		HBox buttonsPane = new HBox(10, chooseButton);

		mainPane.add(sliceBox, 0, 0);
		mainPane.add(buttonsPane, 0, 1);
	}

	private void showChooser() {
		DirectoryChooser chooser = new DirectoryChooser();
		chooser.setTitle("Select folder to save image images");
		File dir = chooser.showDialog(window);
		if (dir.isDirectory()) {
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
			};
		}
	}

	private void createSliceView(int index) {
		ImageView image = new ImageView(slices.images.get(index));

		Node view = sliceBox.getChildren().get(slices.xIndices.get(index));
		if (view != null && view instanceof VBox) {
			((VBox) view).getChildren().add(slices.yIndices.get(index), image);
		}
		else {
			sliceBox.getChildren().add(slices.xIndices.get(index), new VBox(1,image));
		}
	}

	public void addSliceImage(Image image) {
		int x = slices.xIndices.get(selectedSliceIndex) + 1;
		int y = 0;

		slices.images.add(image);
		slices.xIndices.add(x);
		slices.yIndices.add(y);

		selectedSliceIndex = slices.images.size() - 1;
		createSliceView(selectedSliceIndex);
	}
}
