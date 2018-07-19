package linecompare;

import javafx.scene.image.Image;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;

import java.util.List;

public class Slice {

	public Image image;
	public StackPane wrapperView;

	public Slice(Image image, StackPane wrapperView) {
		this.image = image;
		this.wrapperView = wrapperView;
	}

	public List column;
	public VBox columnView;
}
