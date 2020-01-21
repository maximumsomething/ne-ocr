package linecompare;

import javafx.collections.FXCollections;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Pos;
import javafx.scene.control.Button;
import javafx.scene.control.ComboBox;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.TextArea;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.image.PixelReader;
import javafx.scene.image.WritableImage;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.StackPane;
import javafx.scene.paint.Color;
import javafx.scene.shape.Rectangle;
import javafx.stage.DirectoryChooser;
import javafx.stage.FileChooser;
import javafx.stage.Window;
import org.apache.pdfbox.pdmodel.PDDocument;

import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.util.Arrays;
import java.util.prefs.Preferences;

public class SourceViewer {
	public Window window;

	public GridPane mainPane = new GridPane();
	private ImageView pageView = new ImageView();
	private Rectangle selectRect = new Rectangle(0, 0, 0, 0);
	private Image pageImage;
	ComboBox imageSel = new ComboBox();

	// one of these is null
	private PDDocument doc;
	private File imageDirectory;
	private File[] imageFiles;

	private TextArea errorMessage;

	private int pageNum;
	private double zoom = 1;

	private Preferences prefs = Preferences.userRoot().node("SourceViewer");
	private static final String LOCATION_PREF = "book location";

	private double selectX, selectY;

	SourceViewer(Window window) {
		this.window = window;
		setupLayout();
	}

	private void setupLayout() {

		mainPane.setAlignment(Pos.TOP_CENTER);
		mainPane.setHgap(10);
		mainPane.setVgap(10);
		//mainPane.setPadding(new Insets(25, 25, 25, 25));

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
				chooseDir(false);
			}
		});

		Button selectProgramBtn = new Button("Select program");
		selectProgramBtn.setOnAction(event -> {
			chooseDir(true);
		});

		HBox buttonsPane = new HBox(10, leftBtn, imageSel, rightBtn, zoomInBtn, zoomOutBtn, chooseBtn);
		buttonsPane.setAlignment(Pos.CENTER);
		pageView.setPreserveRatio(true);

		selectRect.setStroke(Color.BLUE);
		selectRect.setFill(Color.TRANSPARENT);
		selectRect.setStrokeWidth(2);
		selectRect.setVisible(false);
		StackPane pageStack = new StackPane(pageView, selectRect);
		pageStack.setAlignment(Pos.TOP_LEFT);

		pageStack.addEventFilter(MouseEvent.ANY, mouseEvent -> {

			if (mouseEvent.getEventType() == MouseEvent.MOUSE_PRESSED) {
				selectRect.setVisible(true);
				selectX = mouseEvent.getX();
				selectY = mouseEvent.getY();
			}
			if (mouseEvent.getEventType() == MouseEvent.MOUSE_DRAGGED && selectRect.isVisible()) {
				updateSelectRect(mouseEvent.getX(), mouseEvent.getY(), false);
			}
			if (mouseEvent.getEventType() == MouseEvent.MOUSE_RELEASED) {
				updateSelectRect(mouseEvent.getX(), mouseEvent.getY(), true);
			}
		});

		ScrollPane scroll = new ScrollPane(pageStack);

		mainPane.add(buttonsPane, 0, 0);
		mainPane.add(scroll, 0, 1);

		String prevFile = prefs.get(LOCATION_PREF, "");
		if (!prevFile.equals("")) useDir(new File(prevFile));
	}

	private void updateSelectRect(double mouseX, double mouseY, boolean done) {
		double x = selectX, y = selectY, width = mouseX - selectX, height = mouseY - selectY;
		if (width < 0) {
			x += width;
			width = -width;
		}
		if (height < 0) {
			y += height;
			height = -height;
		}
		if (!done) {
			selectRect.setTranslateX(x);
			selectRect.setTranslateY(y);

			selectRect.setWidth(width);
			selectRect.setHeight(height);
		}
		else {
			selectRect.setVisible(false);

			Image selectedImage = getAreaImage((int) Math.round(x / zoom),
					(int) Math.round(y / zoom),
					(int) Math.round(width / zoom),
					(int) Math.round(height / zoom));
			pushCharacter(selectedImage);

			selectRect.setWidth(0);
			selectRect.setHeight(0);
		}
	}

	private Image getAreaImage(int x, int y, int width, int height) {

		PixelReader reader = pageImage.getPixelReader();
		return new WritableImage(reader, x, y, width, height);
	}

	private void pushSlice(Image slice) {
		Main.sliceViewer.addSliceImage(slice);
	}
	private void pushCharacter(Image character) {
		Main.resultsViewer.useImageFromDoc(character);
	}

	public boolean hasDoc() {
		return doc != null || imageDirectory != null;
	}

	public void chooseDir(boolean isProgramDir) {
		if (!isProgramDir) {
			doc = null;
			imageDirectory = null;
		}

		DirectoryChooser chooser = new DirectoryChooser();
		if (isProgramDir) chooser.setTitle("Select directory with MATLAB program and files");
		else chooser.setTitle("Select folder with images (JPG, PNG, or GIF)");

		File dir = chooser.showDialog(window);

		if (dir != null && dir.isDirectory()) {

			if (isProgramDir) Main.programDir = dir;
			else useDir(dir);


		}
	}

	private void useDir(File dir) {
			imageDirectory = dir;
			imageFiles = imageDirectory.listFiles(new FileFilter() {
				public boolean accept(File pathname) {
					String ucname = pathname.getName().toUpperCase();
					return !ucname.startsWith(".")
							&& (ucname.endsWith(".JPG")
					|| ucname.endsWith(".JPEG")
					|| ucname.endsWith(".JIF")
					|| ucname.endsWith(".PNG")
					|| ucname.endsWith(".GIF"));
				}
			});
			Arrays.sort(imageFiles);
			loadPage();

			String[] fileNames = new String[imageFiles.length];
			for (int i = 0; i < imageFiles.length; ++i) {
				fileNames[i] = imageFiles[i].getName();
			}
			imageSel.setItems(FXCollections.observableArrayList(fileNames));

			try {
				prefs.put(LOCATION_PREF, dir.getCanonicalPath());
			}
			catch (IOException e) {}
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

	private void loadPage() {
		if (imageFiles != null && imageFiles.length > 0) {

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
