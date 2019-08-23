package linecompare;

import com.mathworks.engine.MatlabEngine;
import javafx.embed.swing.SwingFXUtils;
import javafx.scene.image.Image;

import javax.imageio.ImageIO;
import java.io.*;
import java.nio.file.Files;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.stream.Collectors;

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

				String skelCommand = "imwrite(scaleSkel(imread('" +
						tempFile.getAbsolutePath() + "'),  36, 1/3, 0), '" +
						bwimageTempFile + "', 'png')";
				System.out.println(skelCommand);
				engine.eval(skelCommand);

				callback.bwImage(new Image(new File(bwimageTempFile).toURI().toString()));

				System.out.println("skeleton: " + bwimageTempFile);

				String coresPath = Main.programDir.getAbsolutePath() + File.separator + "Working Files" + File.separator + "Extracted Characters" + File.separator + "cores";

				String compareExe = "/Users/max/Library/Developer/Xcode/DerivedData/connection_compare-azguassdtbouupaqtonbeyldvspy/Build/Products/Debug/connection compare";

				String command = "'" + compareExe + "' '" + coresPath + "' '" + bwimageTempFile + "'";
				System.out.println(command);

				ProcessBuilder builder = new ProcessBuilder()
						//.command(compareExe, coresPath, bwimageTempFile)
						//.command("printenv")
						.command("sh", "-c", command);
						//.redirectError(ProcessBuilder.Redirect.INHERIT);
						//.inheritIO();
				Process process = builder.start();

				String charNames = new BufferedReader(new InputStreamReader(process.getInputStream()))
						.lines().collect(Collectors.joining("\n"));
				/*StringBuilder sb = new StringBuilder();
				BufferedReader br = new BufferedReader(new InputStreamReader(process.getInputStream()));
				String read;
				while ((read=br.readLine()) != null) {
					//System.out.println(read);
					sb.append(read);
				}
				String charNames = sb.toString();*/


				/*System.out.println("findChar('" + tempFile.getAbsolutePath() + "', '" + coresPath + "')");

				String charNames = engine.feval(
						"findChar", commandOutput, commandOutput,
						bwimageTempFile, coresPath, true);*/
				process.waitFor();
				System.out.println("chars:\n" + charNames);

				callback.finished(charNames);
			}
			catch (InterruptedException e) {
				return;
			}
			catch (ExecutionException e) {
				e.printStackTrace();
				callback.finished("");
			}
			catch (IOException e) {
				e.printStackTrace();
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
