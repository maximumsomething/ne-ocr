package linecompare;

import javafx.embed.swing.SwingFXUtils;
import javafx.scene.image.Image;

import javax.imageio.ImageIO;
import java.io.*;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

public class ExternalCaller {
	private File tmpDir;
	private Thread callerThread;
	private Process externalProcess;

	public volatile double dotSize = 0.03, holeSize = 0.015;

	enum State {
		SKELETONIZING,
		RESKELETONIZING,
		COMPARING,
		IDLE
	}
	volatile State state = State.IDLE;

	public Callback callback;
	public Image character;

	ExternalCaller() {

		try {
			tmpDir = Files.createTempDirectory("YiDictTool").toFile();
		}
		catch (IOException e) {
			System.out.println("Could not access file system");
			System.exit(1);
		}
	}

	public interface Callback {
		void bwImage(Image bwimage);
		void finished(String charNames);
		void failed();
	}

	public boolean running() {
		return state != State.IDLE;
	}

	public void stop() {
		if (state != State.IDLE) callerThread.interrupt();
		if (externalProcess != null) externalProcess.destroy();
	}

	/*void printCommand(List<String> command) {
		String cmdString = "";
		for (String arg : command) {
			cmdString += "'" + arg + "' ";
		}
		System.out.println(cmdString);
	}*/

	private File getCharsDir() {
		return new File(Main.programDir, "Extracted Characters");
	}
	private String escapeForSh(String in) {
		return "'" + in.replace("'", "\\'") + "'";
	}
	// If the process is called directly instead of through the shell, there are dyld errors.
	private List<String> callThroughShell(List<String> command) {
		String cmdString = "";
		for (String arg: command) {
			cmdString += escapeForSh(arg) + " ";
		}
		System.out.println(cmdString);
		return Arrays.asList("sh", "-c", cmdString);
	}

	public void changeSkelParams(double dotSize, double holeSize) {
		synchronized (this) {
			this.dotSize = dotSize;
			this.holeSize = holeSize;
			if (state == State.SKELETONIZING) state = State.RESKELETONIZING;
			else findChar();
		}
	}

	/*// This class is needed so that if the params change, the previous skeleton will be waited for before starting a new one, but selecting a new character will terminate the current computation.
	private class SkeletonizerCaller {
		public double dotSize, holeSize;
		public String externalExe, imgPath, skelPath;
		ExternalCaller.Callback callback;
		SkeletonizerCaller(double dotSize, double holeSize, String externalExe, String imgPath, String skelPath, ExternalCaller.Callback callback) {
			this.dotSize = dotSize;
			this.holeSize = holeSize;
			this.externalExe = externalExe;
			this.imgPath = imgPath;
			this.skelPath = skelPath;
			this.callback = callback;
		}

		void go() throws IOException, InterruptedException {
			List<String> skelCommand = Arrays.asList(
					externalExe,
					"skeletonize",
					"-outSize=200",
					String.format("-dotSize=%f", dotSize),
					String.format("-holeSize=%f", holeSize),
					imgPath,
					skelPath);

			externalProcess = new ProcessBuilder()
					.command(callThroughShell(skelCommand)).inheritIO().start();
			externalProcess.waitFor();

			//callback.bwImage(new Image();

			System.out.println("skeleton: " + skelFile);
		}
		void changeParams() {

		}
	}*/

	public void findChar(Image character, Callback callback) {
		this.character = character;
		this.callback = callback;
		findChar();
	}
	public void findChar() {
		stop();

		callerThread = new Thread(() -> {
			state = State.SKELETONIZING;
			StringWriter commandOutput = new StringWriter();

			File externalExe = new File(Main.programDir, "connection_compare");

			try {

				File imgFile = new File(tmpDir, "imageIn.png");
				File skelFile = new File(tmpDir, "bwimage.png");
				ImageIO.write(SwingFXUtils.fromFXImage(character, null), "png", imgFile);

				System.out.println("character: " + imgFile);

				do {

					List<String> skelCommand;
					synchronized (ExternalCaller.this) {
						state = State.SKELETONIZING;
						// Compute the skeleton
						// If the process is called directly instead of through the shell, there are dyld errors.
						skelCommand = Arrays.asList(
								externalExe.getAbsolutePath(),
								"skeletonize",
								"-outSize=200",
								String.format("-dotSize=%f", dotSize),
								String.format("-holeSize=%f", holeSize),
								imgFile.getAbsolutePath(),
								skelFile.getAbsolutePath());
					}

					externalProcess = new ProcessBuilder()
							.command(callThroughShell(skelCommand)).inheritIO().start();
					externalProcess.waitFor();

					callback.bwImage(new Image(skelFile.toURI().toString()));

					System.out.println("skeleton: " + skelFile);

					if (Thread.interrupted()) throw new InterruptedException();
				} while (state == State.RESKELETONIZING);

				state = State.COMPARING;

				String coresPath = new File(getCharsDir(), "cores").getAbsolutePath();

				List<String> compareCommand = Arrays.asList(
						externalExe.getPath(),
						"compare",
						coresPath,
						skelFile.getPath());


				externalProcess = new ProcessBuilder()
						.command(callThroughShell(compareCommand))
						.redirectError(ProcessBuilder.Redirect.INHERIT)
						.start();

				String charNames = new BufferedReader(new InputStreamReader(externalProcess.getInputStream()))
						.lines().collect(Collectors.joining("\n"));

				externalProcess.waitFor();
				System.out.println("chars:\n" + charNames);

				callback.finished(charNames);
			}
			catch (InterruptedException e) {
				if (externalProcess != null) externalProcess.destroy();
				externalProcess = null;
			}
			/*catch (ExecutionException e) {
				e.printStackTrace();
				callback.failed();
			}*/
			catch (IOException e) {
				e.printStackTrace();
				callback.failed();
			}
			finally {
				state = State.IDLE;
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
		callerThread.start();

		//gotResults("Page 25 - 1.png\nPage 24 - 1.png\nPage 29 - 7.png\nPage 24 - 3.png\nPage 30 - 4.png\nPage 27 - 4.png");
	}
	public void addMatch(String dictChar, Image ogSelection, Image skel) {

		new Thread(() -> {
			String randId = Integer.toString((int) (Math.random() * Integer.MAX_VALUE)) + ".png";

			File matchesDir = new File(new File(getCharsDir(), "Matches"), dictChar);
			File ogsDir = new File(matchesDir, "og imgs");

			matchesDir.mkdirs();
			ogsDir.mkdirs();

			try {

				ImageIO.write(SwingFXUtils.fromFXImage(ogSelection, null), "png", new File(ogsDir, randId));
				ImageIO.write(SwingFXUtils.fromFXImage(skel, null), "png", new File(matchesDir, randId));
			}
			catch (IOException e) {
				// TODO: handle error
				e.printStackTrace();
			}
		}).start();
	}
}


