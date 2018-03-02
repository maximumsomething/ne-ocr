package linecompare;

import javafx.event.EventHandler;
import javafx.stage.Stage;
import javafx.stage.WindowEvent;

import java.util.prefs.Preferences;

public class PosRestoreStage extends Stage {
	private static final String WINDOW_POSITION_X = "Window_Position_X";
	private static final String WINDOW_POSITION_Y = "Window_Position_Y";
	private static final String WINDOW_WIDTH = "Window_Width";
	private static final String WINDOW_HEIGHT = "Window_Height";

	private Preferences prefs;

	public void restoreSize(String name) {
		prefs = Preferences.userRoot().node("WINDOW_RESTORE_" + name);

		setX(prefs.getDouble(WINDOW_POSITION_X, getX()));
		setY(prefs.getDouble(WINDOW_POSITION_Y, getY()));
		setWidth(prefs.getDouble(WINDOW_WIDTH, getWidth()));
		setHeight(prefs.getDouble(WINDOW_HEIGHT, getHeight()));

		setOnCloseRequest(new EventHandler<WindowEvent>() {
			public void handle(WindowEvent event) {
				prefs.putDouble(WINDOW_POSITION_X, getX());
				prefs.putDouble(WINDOW_POSITION_Y, getY());
				prefs.putDouble(WINDOW_WIDTH, getWidth());
				prefs.putDouble(WINDOW_HEIGHT, getHeight());
			}
		});
	}
}
