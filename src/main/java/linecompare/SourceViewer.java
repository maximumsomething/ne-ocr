package linecompare;

import javafx.application.Platform;
import javafx.collections.FXCollections;
import javafx.embed.swing.SwingFXUtils;
import javafx.event.ActionEvent;
import javafx.event.EventHandler;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.image.PixelReader;
import javafx.scene.image.WritableImage;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.scene.shape.Rectangle;
import javafx.stage.DirectoryChooser;
import javafx.stage.FileChooser;
import javafx.stage.Screen;
import javafx.stage.Window;
import org.apache.pdfbox.pdmodel.PDDocument;
import org.apache.pdfbox.rendering.PDFRenderer;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.prefs.Preferences;

public class SourceViewer {
	public Window window;

	public GridPane mainPane = new GridPane();
	private ImageView pageView = new ImageView();
	private StackPane pageStack;
	private Label imageIndicatorOverlay = new Label();
	private Rectangle selectRect = new Rectangle(0, 0, 0, 0);
	private Image pageImage;
	ComboBox imageSel = new ComboBox();

	// one of these is null
	private PDDocument PDFDoc;
	private File imageDirectory;
	private File[] imageFiles;

	private TextArea errorMessage;

	private int pageNum;
	private int numPages;
	private double zoom = 1;

	private Preferences prefs = Preferences.userRoot().node("SourceViewer");
	private static final String LOCATION_PREF = "book location";
	private static final String PAGE_PREF = "current page";

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
		imageSel.setMaxWidth(Double.MAX_VALUE);
		imageSel.setMinWidth(100);
		imageSel.setPrefWidth(500);

		Button chooseDirBtn = new Button("Choose folder");
		chooseDirBtn.setOnAction(event -> chooseDir(false));

		Button choosePDFBtn = new Button("Choose PDF");
		choosePDFBtn.setOnAction(event -> choosePDF());

		Button selectProgramBtn = new Button("Select program");
		selectProgramBtn.setOnAction(event -> {
			chooseDir(true);
		});

		HBox buttonsPane = new HBox(10, leftBtn, imageSel, rightBtn, zoomInBtn, zoomOutBtn, chooseDirBtn, choosePDFBtn);
		buttonsPane.getChildren().iterator().forEachRemaining((Node btn) -> {
			if (btn instanceof Button) ((Button) btn).setMinSize(Button.USE_PREF_SIZE, Button.USE_PREF_SIZE);
		});
		buttonsPane.setAlignment(Pos.CENTER);
		pageView.setPreserveRatio(true);

		selectRect.setStroke(Color.BLUE);
		selectRect.setFill(Color.TRANSPARENT);
		selectRect.setStrokeWidth(2);
		selectRect.setVisible(false);
		pageStack = new StackPane(pageView, selectRect);
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

		imageIndicatorOverlay.setBackground(new Background(new BackgroundFill(Color.WHITE, CornerRadii.EMPTY, Insets.EMPTY)));
		StackPane scrollStack = new StackPane(scroll, imageIndicatorOverlay);
		scrollStack.setAlignment(Pos.TOP_CENTER);

		mainPane.add(buttonsPane, 0, 0);
		mainPane.add(scrollStack, 0, 1);

		String prevFilePath = prefs.get(LOCATION_PREF, "");
		if (!prevFilePath.equals("")) {
			File prevFile = new File(prevFilePath);

			boolean success = false;
			if (prevFile.isDirectory()) success = useImageDir(prevFile);
			else if (prevFile.isFile()) success = usePDF(prevFile);
			if (success) {
				pageNum = prefs.getInt(PAGE_PREF, 0);
				loadPage();
			}
		}
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
			//selectRect.setVisible(false);

			double realZoom = pageView.getFitWidth() / pageImage.getWidth();
			if (realZoom == 0) realZoom = zoom;

			Image selectedImage = getAreaImage((int) Math.round(x / realZoom),
					(int) Math.round(y / realZoom),
					(int) Math.round(width / realZoom),
					(int) Math.round(height / realZoom));
			pushCharacter(selectedImage);

			//selectRect.setWidth(0);
			//selectRect.setHeight(0);
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
		return PDFDoc != null || imageDirectory != null;
	}

	public void chooseDir(boolean isProgramDir) {
		if (!isProgramDir) removeChosenDoc();


		DirectoryChooser chooser = new DirectoryChooser();
		if (isProgramDir) chooser.setTitle("Select directory with compare executable and files");
		else chooser.setTitle("Select folder with images (JPG, PNG, or GIF)");

		File dir = chooser.showDialog(window);

		if (dir != null && dir.isDirectory()) {

			if (isProgramDir) Main.programDir = dir;
			else {
				useImageDir(dir);
				loadPage();
			}
		}
	}

	private boolean useImageDir(File dir) {
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
		numPages = imageFiles.length;

		String[] fileNames = new String[imageFiles.length];
		for (int i = 0; i < imageFiles.length; ++i) {
			fileNames[i] = imageFiles[i].getName();
		}
		imageSel.setItems(FXCollections.observableArrayList(fileNames));

		try {
			prefs.put(LOCATION_PREF, dir.getCanonicalPath());
		}
		catch (IOException e) {}

		return true;
	}

	public void choosePDF() {
		removeChosenDoc();

		FileChooser chooser = new FileChooser();
		chooser.setTitle("Select PDF");
		chooser.getExtensionFilters().setAll(new FileChooser.ExtensionFilter("PDF", "*.pdf", "*.PDF"));

		File file = chooser.showOpenDialog(window);
		if (file != null && file.isFile()) {
			usePDF(file);
			loadPage();
		}

	}
	public boolean usePDF(File pdfFile) {
		try {
			PDFDoc = PDDocument.load(pdfFile);
		} catch (IOException e) {
			showError();
			return false;
		}
		numPages = PDFDoc.getNumberOfPages();
		String[] pageNames = new String[numPages];
		for (int i = 0; i < numPages; ++i) {
			pageNames[i] = "Page " + Integer.toString(i + 1);
		}
		imageSel.setItems(FXCollections.observableArrayList(pageNames));

		try {
			prefs.put(LOCATION_PREF, pdfFile.getCanonicalPath());
		}
		catch (IOException e) {}

		return true;
	}

	private void showError() {
		if (errorMessage == null) {
			errorMessage = new TextArea("Error opening document");
		}
		mainPane.add(errorMessage, 0, 1);
	}

	// Super ugly hack
	//https://stackoverflow.com/questions/28817460/how-do-i-find-out-whether-my-program-is-running-on-a-retina-screen
	private double getScreenScale(Screen screen) {
		try {
			Method m = Screen.class.getDeclaredMethod("getRenderScale");
			m.setAccessible(true);
			return ((Float) m.invoke(screen)).doubleValue();
		}
		catch (NoSuchMethodException|SecurityException|IllegalAccessException|IllegalArgumentException| InvocationTargetException e) {
			System.err.println("Error getting screen dpi");
			e.printStackTrace();
			return 1;
		}
	}

	Thread pdfRenderThread;

	private void loadPage() {
		if (pageNum < 0) pageNum = 0;
		if (pageNum >= numPages) pageNum = numPages-1;

		imageSel.getSelectionModel().select(pageNum);

		if (imageFiles != null && imageFiles.length > 0) {

			pageImage = new Image(imageFiles[pageNum].toURI().toString());
			pageView.setImage(pageImage);
			imageIndicatorOverlay.setText("");
		}
		else if (PDFDoc != null) {
			imageIndicatorOverlay.setText("Loading...");
			double screenScale = getScreenScale(Screen.getPrimary());
			if (pdfRenderThread != null && pdfRenderThread.isAlive()) {
				// I think this is safe
				pdfRenderThread.stop();
			}
			pdfRenderThread = new Thread(() -> {
				try {
					BufferedImage awtImage = new PDFRenderer(PDFDoc).renderImage(pageNum, (float) (zoom * screenScale));
					Image fxImage = SwingFXUtils.toFXImage(awtImage, null);
					Platform.runLater(() -> {
						pageImage = fxImage;
						pageView.setImage(pageImage);
						updateZoom();
						imageIndicatorOverlay.setText("");
					});
				}
				catch (IOException|RuntimeException e) {
					Platform.runLater(() -> {
						imageIndicatorOverlay.setText("Could not render PDF");
					});
				}
			});
			pdfRenderThread.start();
		}

		prefs.putInt(PAGE_PREF, pageNum);
	}
	private void removeChosenDoc() {
		if (PDFDoc != null) {
			try {
				PDFDoc.close();
			}
			catch (IOException e) {}
			PDFDoc = null;
		}
		imageDirectory = null;
		imageFiles = null;
	}
	private void updateZoom() {
		if (PDFDoc != null) {
			pageView.setFitWidth(PDFDoc.getPage(pageNum).getMediaBox().getWidth() * zoom);
			loadPage();
		}
		else if (pageImage != null) pageView.setFitWidth(pageImage.getWidth() * zoom);
	}
}
