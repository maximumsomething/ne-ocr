package linecompare;

import javafx.collections.FXCollections;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.ComboBox;
import javafx.scene.control.TextArea;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.stage.DirectoryChooser;
import javafx.stage.FileChooser;
import javafx.stage.Window;
import org.apache.pdfbox.pdmodel.PDDocument;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;

public class SourceViewer {
	public Window window;

	public GridPane mainPane = new GridPane();
	private ImageView pageView = new ImageView();
	private Image pageImage;
	ComboBox imageSel = new ComboBox();

	// one of these is null
	private PDDocument doc;
	private File imageDirectory;
	private File[] imageFiles;

	private TextArea errorMessage;

	private int pageNum;
	private double zoom = 1;

	SourceViewer(Window window) {
		this.window = window;
		setupLayout();
	}

	private void setupLayout() {

		mainPane.setAlignment(Pos.TOP_CENTER);
		mainPane.setHgap(10);
		mainPane.setVgap(10);
		mainPane.setPadding(new Insets(25, 25, 25, 25));

		Button leftBtn = new Button("<<");
		leftBtn.setOnAction(new EventHandler<ActionEvent>() {
			public void handle(ActionEvent event) {
				pageNum--;
				loadPage();
			}
		});

		Button rightBtn = new Button(">>");
		rightBtn.setOnAction(new EventHandler<ActionEvent>() {
			public void handle(ActionEvent event) {
				pageNum++;
				loadPage();
			}
		});

		Button zoomInBtn = new Button("+");
		zoomInBtn.setOnAction(new EventHandler<ActionEvent>() {
			public void handle(ActionEvent event) {
				zoom *= 1.2;
				updateZoom();
			}
		});

		Button zoomOutBtn = new Button("-");
		zoomOutBtn.setOnAction(new EventHandler<ActionEvent>() {
			public void handle(ActionEvent event) {
				zoom /= 1.2;
				updateZoom();
			}
		});
		imageSel.setOnAction(new EventHandler<ActionEvent>() {
			public void handle(ActionEvent event) {
				pageNum = imageSel.getItems().indexOf(imageSel.getValue());
				loadPage();
			}
		});

		Button chooseBtn = new Button("Choose folder");
		chooseBtn.setOnAction(new EventHandler<ActionEvent>() {
			public void handle(ActionEvent event) {
				chooseDir();
			}
		});

		HBox buttonsPane = new HBox(10, leftBtn, imageSel, rightBtn, zoomInBtn, zoomOutBtn, chooseBtn);
		buttonsPane.setAlignment(Pos.CENTER);
		pageView.setPreserveRatio(true);

		mainPane.add(buttonsPane, 0, 0);
		mainPane.add(pageView, 0, 1);
	}

	public boolean hasDoc() {
		return doc != null || imageDirectory != null;
	}

	public void chooseDir() {
		doc = null;
		imageDirectory = null;

		DirectoryChooser chooser = new DirectoryChooser();
		chooser.setTitle("Select folder with images");

		File dir = chooser.showDialog(window);
		if (dir != null) {
			imageDirectory = dir;
			imageFiles = imageDirectory.listFiles(new DotUnderscoreFilter());
			Arrays.sort(imageFiles);
			loadPage();

			String[] fileNames = new String[imageFiles.length];
			for (int i = 0; i < imageFiles.length; ++i) {
				fileNames[i] = imageFiles[i].getName();
			}
			imageSel.setItems(FXCollections.observableArrayList(fileNames));
		}
	}

	public void choosePDF(Window window) {
		doc = null;
		imageDirectory = null;

		FileChooser chooser = new FileChooser();
		chooser.setTitle("Select PDF");

		File file = chooser.showOpenDialog(window);

		try {
			doc = PDDocument.load(file);
		} catch (IOException e) {
			showError();
		}

		loadPage();
	}

	private void showError() {
		if (errorMessage == null) {
			errorMessage = new TextArea("Error opening document");
		}
		mainPane.add(errorMessage, 0, 1);
	}

	private class DotUnderscoreFilter implements FilenameFilter {

		public boolean accept(File dir, String name) {
			return !name.startsWith("._");
		}
	}

	private void loadPage() {
		if (imageFiles != null) {

			if (pageNum < 0) pageNum = 0;
			if (pageNum > imageFiles.length) pageNum = imageFiles.length;

			pageImage = new Image(imageFiles[pageNum].toURI().toString());
			pageView.setImage(pageImage);
		}
		else {

		}
	}
	private void updateZoom() {
		if (pageImage != null) pageView.setFitWidth(pageImage.getWidth() * zoom);
	}
}
