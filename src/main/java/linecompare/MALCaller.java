package linecompare;

import com.mathworks.engine.MatlabEngine;
import javafx.embed.swing.SwingFXUtils;
import javafx.scene.image.Image;

import javax.imageio.ImageIO;
import java.io.File;
import java.io.IOException;
import java.io.StringWriter;
import java.nio.file.Files;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;

public class MALCaller {
	private File tmpDir;
	private Future<MatlabEngine> engineFuture;
	private Thread matlabThread;

	public boolean running = false;

	MALCaller() {

		try {
			tmpDir = Files.createTempDirectory("YiDictTool").toFile();
		}
		catch (IOException e) {
			System.out.println("Could not access file system");
			System.exit(1);
		}

		engineFuture = MatlabEngine.startMatlabAsync();
	}

	public interface Callback {
		void bwImage(Image bwimage);
		void finished(String charNames);
	}

	void findChar(Image character, Callback callback) {

		if (running) matlabThread.interrupt();
			/*
			Process process = new ProcessBuilder(
					Main.programDir.getAbsolutePath() + File.separator + "findchar",
					tempFile.getAbsolutePath()).start();*/


		matlabThread = new Thread(() -> {
			running = true;
			StringWriter commandOutput = new StringWriter();

			try {
				File tempFile = new File(tmpDir.getPath() + File.separator + "imageIn.png");
				String bwimageTempFile = tmpDir.getPath() + File.separator + "bwimage.png";
				ImageIO.write(SwingFXUtils.fromFXImage(character, null), "png", tempFile);

				MatlabEngine engine = engineFuture.get();

				engine.eval("cd " + Main.programDir.getAbsolutePath());

				// get skeleton to show while computing
				engine.eval("imwrite(scaleSkel(imread('" +
						tempFile.getAbsolutePath() + "'),  12, 1/3, 0), '" +
						bwimageTempFile + "', 'png')");

				callback.bwImage(new Image(new File(bwimageTempFile).toURI().toString()));

				String coresPath = Main.programDir.getAbsolutePath() + File.separator + "Working Files" + File.separator + "Extracted Characters" + File.separator + "cores";


				System.out.println("findChar('" + tempFile.getAbsolutePath() + "', '" + coresPath + "')");


				String charNames = engine.feval(
						"findChar", commandOutput, commandOutput,
						bwimageTempFile, coresPath, true);

				callback.finished(charNames);
			}
			catch (InterruptedException e) {
				return;
			}
			catch (ExecutionException e) {
				callback.finished("");
			}
			catch (IOException e) {
				// TODO
			}
			finally {
				running = false;
			}

			/*try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream()))) {
				ArrayList<String> lines = new ArrayList<>();
				String line;
				while ((line = reader.readLine()) != null) {
					lines.add(line);
				}

				Platform.runLater(() -> gotResults((String[]) lines.toArray()));
				process.waitFor();
			}
			catch (IOException|InterruptedException e) {

			}*/

		});
		matlabThread.start();

		//gotResults("Page 25 - 1.png\nPage 24 - 1.png\nPage 29 - 7.png\nPage 24 - 3.png\nPage 30 - 4.png\nPage 27 - 4.png");


	}
}
