#ifndef FILE_READER_INTERFACE_H
#define FILE_READER_INTERFACE_H
#include <iostream>
#include <string>
#include <sstream>

/**
 * @brief Interface for getting class
 */
class FileReaderInterface
{
public:
  /**
   * Close video file if opened.
   */
	virtual ~FileReaderInterface(){};
  /**
   * Return file open state.
   * @return file open state
   */
	virtual bool isOpen() = 0;
  /**
   * Open video file.
   * @param v_file video file path
   * @param width width of loaded video or 0 if fail
   * @param height height of loaded video or 0 if fail
   * @param error optional string to write error
   * @return success of open file
   */
	virtual bool openFile(std::string &v_file, int &width, int &height, std::stringstream *error = NULL) = 0;
  /**
   * Close video file if opened.
   */
	virtual void closeFile() = 0;

  virtual bool setFramePos(unsigned int frame_pos) = 0;
  /**
   * Get next frame.
   * @param bgra_frame output frame
   * @param error optional string to write error
   * @return true if return regular frame or false if reach end of file
   */
	virtual bool getFrame(void **frame, std::stringstream *error = NULL) = 0;
};

#endif

