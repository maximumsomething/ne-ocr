package linecompare;

import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.image.Image;
import javafx.scene.input.MouseEvent;
import javafx.scene.layout.Pane;
import javafx.scene.layout.Region;
import javafx.scene.paint.Color;

import java.util.ArrayList;

public class DrawingView extends Pane {

	Canvas canvas = new Canvas(150, 150);
	private double prevMouseX, prevMouseY;

	private ArrayList<Image> undoStack = new ArrayList<>();
	int placeInUndoStack = -1;


	public boolean isActive;

	public interface StrokeFinishedHandler {
		void onStrokeFinished(Image img);
	}
	public StrokeFinishedHandler onStrokeFinished;

	DrawingView() {
		GraphicsContext gc = canvas.getGraphicsContext2D();
		gc.setStroke(Color.BLUE);
		gc.setLineWidth(6);

		this.setStyle("-fx-border-color: black");
		this.setMinSize(Region.USE_PREF_SIZE, Region.USE_PREF_SIZE);
		this.setMaxSize(Region.USE_PREF_SIZE, Region.USE_PREF_SIZE);

		canvas.addEventFilter(MouseEvent.ANY, mouseEvent -> {
			if (isActive) {
				if (mouseEvent.isPrimaryButtonDown()) {
					gc.strokeLine(prevMouseX, prevMouseY, mouseEvent.getX(), mouseEvent.getY());
				}
				if (mouseEvent.getEventType() == MouseEvent.MOUSE_RELEASED) {

					if (placeInUndoStack < undoStack.size() - 1)
						undoStack.subList(placeInUndoStack + 1, undoStack.size() - 1).clear();

					undoStack.add(getImage());
					placeInUndoStack = undoStack.size() - 1;

					if (onStrokeFinished != null) onStrokeFinished.onStrokeFinished(getImage());
				}
				prevMouseX = mouseEvent.getX();
				prevMouseY = mouseEvent.getY();
			}
		});

		this.getChildren().add(canvas);
	}
	public Image getImage() {
		return canvas.snapshot(null, null);
	}
	private void resetFromUndoStack() {
		canvas.getGraphicsContext2D().drawImage(undoStack.get(placeInUndoStack), 0, 0);
		if (onStrokeFinished != null) onStrokeFinished.onStrokeFinished(undoStack.get(placeInUndoStack));
	}
	public void undo() {
		--placeInUndoStack;
		resetFromUndoStack();

	}
	public void redo() {
		if (placeInUndoStack < undoStack.size() - 1) {
			++placeInUndoStack;
			resetFromUndoStack();
		}
	}
	public void clear() {
		canvas.getGraphicsContext2D().setFill(Color.WHITE);
		canvas.getGraphicsContext2D().fillRect(0, 0, 150,150);
	}
}
