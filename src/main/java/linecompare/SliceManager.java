package linecompare;

import java.io.File;
import java.io.FileFilter;
import java.io.IOException;

public class SliceManager {
	// list of columns

	public File sliceDirectory;

	public void loadSlices() throws IOException {

		File[] files = sliceDirectory.listFiles(new FileFilter() {
			public boolean accept(File pathname) {
				return !pathname.getName().startsWith(".");
			}
		});

		/*images = new ArrayList<Image>(files.length);
		xIndices = new ArrayList<Integer>(files.length);
		yIndices = new ArrayList<Integer>(files.length);*/

		for (File i : files) {
			try {
				//xIndices.add(Integer.decode(i.getName()));
				//images.add(new Image(i.toURI().toString()));
			}
			catch (NumberFormatException e) {}
		}
	}
}
