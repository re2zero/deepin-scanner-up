#include "scannerdevice.h"
#include <QDebug>
#include <QTemporaryFile>
#include <cstdio> // for fdopen

QImage ScannerDevice::getPreviewImage()
{
    if (!m_deviceOpen) {
        qWarning() << "Cannot get preview - no device open";
        return QImage();
    }
    
#ifndef _WIN32
    // 创建一个临时文件来存储预览图像
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        qWarning() << "Failed to create temp file for preview";
        return QImage();
    }
    
    // 使用低分辨率快速扫描预览
    FILE* file = fdopen(tempFile.handle(), "w+");
    if (!file) {
        qWarning() << "Failed to open temp file for preview";
        return QImage();
    }
    
    SANE_Status status = scan_it(file);
    fclose(file);
    if (status != SANE_STATUS_GOOD) {
        qWarning() << "Preview scan failed with status:" << status;
        return QImage();
    }
    
    // 读取临时文件内容并创建QImage
    tempFile.seek(0);
    QImage preview;
    if (!preview.load(&tempFile, "PNG")) {
        qWarning() << "Failed to load preview image from temp file";
        return QImage();
    }
    
    return preview;
#else
    qWarning() << "Preview not supported on Windows";
    return QImage();
#endif
}
#include <QDebug>
#include <QFile>
#include <QApplication> // For applicationDirPath
#include <QDir>         // For separator

// --- Dummy definitions for Image/advance if not available elsewhere ---
// Replace these with your actual definitions if they exist elsewhere
#ifndef _WIN32
bool ScannerDevice::advance(struct ScannerDevice::Image* im) {
    // Dummy implementation - replace with your actual 'advance' logic
    qWarning() << "Warning: Dummy 'advance' function called. Implement required logic.";
    if (!im || !im->data) return false; // Indicate failure if im or data is null

    // Basic placeholder logic:
    im->x++;
    if (im->x >= im->width) {
        im->x = 0;
        im->y++;
        // Minimal reallocation simulation check (needs proper implementation)
        if (im->y >= im->height) {
            // Need reallocation logic here if height grows dynamically
            qWarning() << "Image buffer full in dummy 'advance'. Reallocation needed.";
            // Here, your actual advance might realloc 'im->data' and update 'im->height'.
            // If reallocation fails, return false. If successful, return true.
            return false; // Indicate failure (buffer full and no reallocation)
        }
    }
    // Indicate success if buffer is still valid (potentially after reallocation in real code)
    return true;
}
// Dummy implementation for the write_png_header function referenced in scan_it
// You will need the actual implementation.
static void write_png_header(SANE_Frame format, int width, int height, int depth, FILE *ofp, png_structp *png_ptr_p, png_infop *info_ptr_p) {
    qWarning() << "Warning: Dummy 'write_png_header' function called.";
    // Minimal setup to avoid crashes in png_write_row/end if called
    *png_ptr_p = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!*png_ptr_p) return;
    *info_ptr_p = png_create_info_struct(*png_ptr_p);
    if (!*info_ptr_p) {
        png_destroy_write_struct(png_ptr_p, nullptr);
        return;
    }
    if (setjmp(png_jmpbuf(*png_ptr_p))) { // Basic error handling setup
       png_destroy_write_struct(png_ptr_p, info_ptr_p);
       return;
    }
    png_init_io(*png_ptr_p, ofp);
    // Set basic header fields (replace with actual logic from your original code)
    png_set_IHDR(*png_ptr_p, *info_ptr_p, width, height, depth,
                 (format == SANE_FRAME_RGB) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(*png_ptr_p, *info_ptr_p);
}
#endif
// --- End Dummy definitions ---


ScannerDevice::ScannerDevice(QObject *parent) : QObject(parent)
{
#ifdef _WIN32
    qWarning() << "SANE scanning is not supported on Windows in this build.";
#endif
}

ScannerDevice::~ScannerDevice()
{
#ifndef _WIN32
    closeDevice(); // Ensure device is closed if open
    if (m_saneInitialized) {
        sane_exit();
        qDebug() << "SANE exited.";
    }
#endif
}

bool ScannerDevice::initializeSane()
{
#ifndef _WIN32
    if (m_saneInitialized) {
        return true;
    }

    SANE_Int version_code;
    SANE_Status status = sane_init(&version_code, NULL); // Using NULL for auth_callback
    if (status != SANE_STATUS_GOOD) {
        QString errorMsg = QString("Failed to initialize SANE: %1").arg(sane_strstatus(status));
        qWarning() << errorMsg;
        emit scanError(errorMsg);
        return false;
    }
    m_saneInitialized = true;
    qDebug() << "SANE initialized successfully. Version code:" << version_code;
    return true;
#else
    emit scanError("SANE backend not available on this platform.");
    return false;
#endif
}

QStringList ScannerDevice::getAvailableDevices()
{
    QStringList deviceNames;
#ifndef _WIN32
    if (!m_saneInitialized) {
        if (!initializeSane()) {
            return deviceNames; // Return empty list on init failure
        }
    }

    const SANE_Device **device_list;
    SANE_Status status = sane_get_devices(&device_list, SANE_FALSE); // SANE_FALSE = local only
    if (status != SANE_STATUS_GOOD) {
        QString errorMsg = QString("Failed to get SANE device list: %1").arg(sane_strstatus(status));
        qWarning() << errorMsg;
        emit scanError(errorMsg);
        return deviceNames;
    }

    qDebug() << "Available SANE devices:";
    for (int i = 0; device_list[i]; ++i) {
        if (device_list[i]->name) {
            qDebug() << " - " << device_list[i]->name << ":" << device_list[i]->vendor << ":" << device_list[i]->model;
            deviceNames.append(device_list[i]->name);
        }
    }

    if (deviceNames.isEmpty()) {
         qDebug() << "No SANE devices found.";
         // Don't emit an error here, just return empty list
    }

#else
     emit scanError("SANE backend not available on this platform.");
#endif
    return deviceNames;
}

bool ScannerDevice::openDevice(const QString &deviceName)
{
#ifndef _WIN32
    if (!m_saneInitialized) {
        emit scanError("SANE not initialized. Call initializeSane() first.");
        return false;
    }
    if (m_deviceOpen) {
        closeDevice(); // Close previous device if any
    }

    QByteArray deviceNameBytes = deviceName.toUtf8(); // SANE uses const char*
    SANE_Status status = sane_open(deviceNameBytes.constData(), &m_device);
    if (status != SANE_STATUS_GOOD) {
        QString errorMsg = QString("Failed to open SANE device '%1': %2")
                               .arg(deviceName)
                               .arg(sane_strstatus(status));
        qWarning() << errorMsg;
        emit scanError(errorMsg);
        m_device = nullptr;
        m_deviceOpen = false;
        return false;
    }

    // Set I/O mode (optional, 0 usually means blocking)
    status = sane_set_io_mode(m_device, SANE_FALSE); // SANE_FALSE for blocking mode
     if (status != SANE_STATUS_GOOD) {
        qWarning() << "Failed to set SANE I/O mode:" << sane_strstatus(status) << "- proceeding anyway.";
        // Decide if this is critical - often it's not.
    }

    // Optionally get and log parameters
    SANE_Parameters pars;
    status = sane_get_parameters(m_device, &pars);
     if (status != SANE_STATUS_GOOD) {
        qWarning() << "Failed to get initial SANE parameters:" << sane_strstatus(status);
        // Decide if this is critical
     } else {
         qDebug() << "Device" << deviceName << "opened. Format:" << pars.format << " Pixels/Line:" << pars.pixels_per_line;
     }

    m_deviceOpen = true;
    qDebug() << "Device" << deviceName << "opened successfully.";
    return true;

#else
    emit scanError("SANE backend not available on this platform.");
    return false;
#endif
}

void ScannerDevice::closeDevice()
{
#ifndef _WIN32
    if (m_deviceOpen && m_device) {
        sane_close(m_device);
        qDebug() << "Closed SANE device.";
    }
    m_device = nullptr;
    m_deviceOpen = false;
#endif
}

void ScannerDevice::startScan(const QString &tempOutputFilePath)
{
#ifndef _WIN32
    if (!m_deviceOpen || !m_device) {
        emit scanError("No SANE device is open.");
        return;
    }

    // Set scan parameters if device supports it
    if (m_deviceOpen && m_device) {
        // Get option descriptor to check available options
        const SANE_Option_Descriptor *opt;
        
        // Try to set resolution (DPI)
        opt = sane_get_option_descriptor(m_device, 0); // Typically option 0 is resolution
        if (opt && opt->type == SANE_TYPE_INT) {
            SANE_Int resolution = m_dpi;
            sane_control_option(m_device, 0, SANE_ACTION_SET_VALUE, &resolution, nullptr);
        }
        
        // Try to set color mode
        opt = sane_get_option_descriptor(m_device, 1); // Typically option 1 is mode
        if (opt && opt->type == SANE_TYPE_STRING) {
            QByteArray mode = m_colorMode.toUtf8();
            sane_control_option(m_device, 1, SANE_ACTION_SET_VALUE, mode.data(), nullptr);
        }
    }

    // Start the scan frame
    SANE_Status status;
#ifdef SANE_STATUS_WARMING_UP // Handle warm-up if constant exists
    do {
        status = sane_start(m_device);
        if (status == SANE_STATUS_WARMING_UP) {
             qDebug() << "Scanner warming up...";
             // Consider adding a small delay or emitting a status update
             QThread::msleep(500); // Example delay
        }
    } while (status == SANE_STATUS_WARMING_UP);
#else
    status = sane_start(m_device);
#endif

    if (status != SANE_STATUS_GOOD) {
        QString errorMsg = QString("sane_start failed: %1").arg(sane_strstatus(status));
        qWarning() << errorMsg;
        emit scanError(errorMsg);
        // Consider closing the device here? Or leave it for the user?
        // closeDevice();
        return;
    }

    qDebug() << "Scan started. Writing temporary PNG to:" << tempOutputFilePath;

    // Open the output file pointer for scan_it
    QByteArray tempPathBytes = tempOutputFilePath.toLocal8Bit(); // Use local encoding for file paths
    FILE *ofp = fopen(tempPathBytes.constData(), "wb"); // Use "wb" for binary PNG
    if (!ofp) {
        QString errorMsg = QString("Failed to open temporary output file '%1' for writing.").arg(tempOutputFilePath);
        qWarning() << errorMsg;
        emit scanError(errorMsg);
        sane_cancel(m_device); // Cancel the scan started above
        return;
    }

    // --- Call the core scanning function ---
    status = scan_it(ofp);
    // ---                                ---

    fclose(ofp); // Close the file pointer

    if (status != SANE_STATUS_GOOD && status != SANE_STATUS_EOF) {
        // EOF is expected at the end of a successful scan, so don't treat it as an error here.
         QString errorMsg = QString("Scan failed during read: %1").arg(sane_strstatus(status));
         qWarning() << errorMsg;
         emit scanError(errorMsg);
         QFile::remove(tempOutputFilePath); // Clean up temp file on error
         // sane_cancel(m_device); // scan_it might have implicitly cancelled or finished
    } else {
        qDebug() << "scan_it finished with status:" << sane_strstatus(status);
        // Load the QImage from the temporary file
        QImage scannedImage;
        if (scannedImage.load(tempOutputFilePath)) {
             qDebug() << "Successfully loaded scanned image from temp file.";
             emit scanCompleted(scannedImage);
        } else {
             QString errorMsg = QString("Failed to load QImage from temporary file '%1'.").arg(tempOutputFilePath);
             qWarning() << errorMsg;
             emit scanError(errorMsg);
        }
        // Clean up the temporary file
        if (!QFile::remove(tempOutputFilePath)) {
            qWarning() << "Could not remove temporary scan file:" << tempOutputFilePath;
        }
    }

    // Note: sane_cancel or sane_close is implicitly handled by scan_it's cleanup
    // or should be called in closeDevice(). We don't call sane_exit here,
    // leave it for the destructor.

#else
    emit scanError("SANE backend not available on this platform.");
#endif
}

void ScannerDevice::setScanParams(int dpi, const QString &colorMode)
{
    m_dpi = dpi;
    m_colorMode = colorMode;
    qDebug() << "Scan parameters set - DPI:" << m_dpi << "Color Mode:" << m_colorMode;
}

void ScannerDevice::cancelScan()
{
#ifndef _WIN32
    if (m_deviceOpen && m_device) {
        sane_cancel(m_device);
        qDebug() << "Sent SANE cancel request.";
        // Note: Cancellation might not be immediate or guaranteed.
        // The ongoing sane_read might still complete or return an error status.
    }
#endif
}


// --- scan_it implementation (moved inside the class) ---
#ifndef _WIN32
SANE_Status ScannerDevice::scan_it(FILE *ofp)
{
    // --- This is largely your original scan_it function ---
    // --- Adapted slightly for class context and progress emission ---

    // Constants from original code - adjust if needed
    const int STRIP_HEIGHT = 128; // Example value, check original context

    int i, len, first_frame = 1, offset = 0, must_buffer = 0, hundred_percent;
    SANE_Parameters parm;
    SANE_Status status = SANE_STATUS_GOOD; // Initialize status
    Image image = { nullptr, 0, 0, 0, 0, 0 }; // Use class's Image struct

    SANE_Word total_bytes = 0; // Use SANE_Word

    // PNG related variables
    png_structp png_ptr = nullptr;
    png_infop info_ptr = nullptr;
    png_bytep pngbuf = nullptr; // Buffer for one row of PNG data
    int pngrow = 0; // Current position within pngbuf

    SANE_Byte *buffer; // Buffer for sane_read
    size_t buffer_size = (128 * 1024); // Increased buffer size (adjust as needed)
    buffer = (SANE_Byte*)malloc(buffer_size);
    if (!buffer) {
        qWarning() << "Failed to allocate read buffer.";
        return SANE_STATUS_NO_MEM;
    }


    do {
        if (!first_frame) {
#ifdef SANE_STATUS_WARMING_UP
            do {
                status = sane_start(m_device); // Use member variable m_device
                 if (status == SANE_STATUS_WARMING_UP) {
                     qDebug() << "Scanner warming up (multi-frame)...";
                     QThread::msleep(500);
                 }
            } while (status == SANE_STATUS_WARMING_UP);
#else
            status = sane_start(m_device);
#endif
            if (status != SANE_STATUS_GOOD) {
                qWarning() << "sane_start (multi-frame) failed:" << sane_strstatus(status);
                goto cleanup; // Use goto for cleanup as in original
            }
        }

        status = sane_get_parameters(m_device, &parm);
        if (status != SANE_STATUS_GOOD) {
            qWarning() << "sane_get_parameters failed:" << sane_strstatus(status);
            goto cleanup;
        }

        // --- Frame setup (largely same as original) ---
        if (first_frame) {
            qDebug() << "Scan parameters - Format:" << parm.format << "Depth:" << parm.depth
                     << "Lines:" << parm.lines << "Pixels/Line:" << parm.pixels_per_line
                     << "Bytes/Line:" << parm.bytes_per_line;

            // Determine if buffering is needed
             must_buffer = 0; // Reset flag
             if (parm.format >= SANE_FRAME_RED && parm.format <= SANE_FRAME_BLUE) {
                 must_buffer = 1; // Multi-frame requires buffering
                 offset = parm.format - SANE_FRAME_RED;
             } else if (parm.lines < 0) {
                  must_buffer = 1; // Unknown height requires buffering
                  offset = 0;
             }


             if (!must_buffer) {
                // Write PNG header directly if height is known
                 write_png_header(parm.format, parm.pixels_per_line, parm.lines, parm.depth, ofp, &png_ptr, &info_ptr);
                 if (!png_ptr || !info_ptr) {
                     qWarning() << "Failed to write PNG header.";
                     status = SANE_STATUS_IO_ERROR; // Indicate error
                     goto cleanup;
                 }
                 // Allocate buffer for one row
                 pngbuf = (png_bytep)malloc(parm.bytes_per_line);
                 if (!pngbuf) {
                     qWarning() << "Failed to allocate PNG row buffer.";
                     status = SANE_STATUS_NO_MEM;
                     goto cleanup;
                 }
                 pngrow = 0; // Reset row buffer position
             } else {
                 // --- Buffering Logic (Requires proper Image/advance) ---
                 qDebug() << "Buffering scan data (multi-frame or unknown height).";
                 image.width = parm.bytes_per_line;
                 image.height = (parm.lines > 0) ? (parm.lines + 1) : 512; // Initial guess or use parameter
                 image.line_buffer_size = image.width * ((parm.format == SANE_FRAME_RGB && parm.depth > 8) ? 6 : 3); // Rough estimate for RGB16/RGB8
                 size_t initial_alloc = (size_t)image.height * image.line_buffer_size;

                 qDebug() << "Allocating image buffer: width=" << image.width << "initial_height=" << image.height << "bytes=" << initial_alloc;
                 image.data = (unsigned char*)malloc(initial_alloc);

                 if (!image.data) {
                     qWarning() << "Failed to allocate image buffer.";
                     status = SANE_STATUS_NO_MEM;
                     goto cleanup;
                 }
                  // Initialize image state
                 image.x = 0; // Start at the beginning of the first line
                 image.y = 0; // Start at the first line
                 offset = (parm.format >= SANE_FRAME_RED && parm.format <= SANE_FRAME_BLUE) ? (parm.format - SANE_FRAME_RED) : 0;
                 // Reset total bytes for accurate progress if buffering
                 total_bytes = 0;
                 // --- End Buffering Setup ---
             }
        } else { // Not first frame (implies must_buffer was true before)
             // This part assumes multi-frame (R, G, B) and image buffer exists
             assert(parm.format >= SANE_FRAME_RED && parm.format <= SANE_FRAME_BLUE);
             assert(image.data != nullptr);
             offset = parm.format - SANE_FRAME_RED;
             image.x = 0; // Reset position for the new frame/plane
             image.y = 0;
        }

        // Calculate expected bytes for progress (rough estimate if lines < 0)
        SANE_Word expected_bytes = (parm.lines > 0)
            ? (SANE_Word)parm.bytes_per_line * parm.lines
            : 0; // Cannot calculate if lines unknown

         if (must_buffer && parm.format >= SANE_FRAME_RED && parm.format <= SANE_FRAME_BLUE) {
             expected_bytes *= 3; // Expect 3 frames for R,G,B
         }
         hundred_percent = (expected_bytes > 0) ? 100 : -1; // Use -1 if percentage cannot be calculated


        // --- Read loop ---
        while (true) {
            status = sane_read(m_device, buffer, buffer_size, &len);

            if (status != SANE_STATUS_GOOD) {
                if (status == SANE_STATUS_EOF) {
                    qDebug() << "SANE_EOF received.";
                    break; // End of frame/scan
                } else {
                    qWarning() << "sane_read failed:" << sane_strstatus(status);
                    goto cleanup; // Error occurred
                }
            }

            total_bytes += (SANE_Word)len;

            // --- Emit Progress ---
            if (hundred_percent > 0) {
                double progr = ((double)total_bytes * 100.0) / (double)expected_bytes;
                if (progr > 100.0) progr = 100.0;
                emit scanProgress(static_cast<int>(progr));
            } else {
                // Maybe emit based on total bytes if percentage unknown?
                 // emit scanProgress(-1); // Indicate unknown progress
            }


            if (must_buffer) {
                // --- Copy data to image buffer (Requires working advance) ---
                // This section needs careful review based on your 'advance' implementation
                // and how image.data should be structured (planar vs interleaved for RGB)
                 int bytes_per_pixel = (parm.depth > 8) ? 2 : 1;
                 int components = (parm.format == SANE_FRAME_RGB) ? 3 : 1;
                 size_t required_size = (size_t)(image.y + 1) * image.width * bytes_per_pixel * components; // Check calculation

                 // Check if reallocation is needed (simplified)
                if (image.data && required_size > (size_t)image.height * image.line_buffer_size) {
                    // --- Reallocation logic needed here ---
                    qWarning() << "Image buffer reallocation needed but not implemented!";
                    // status = SANE_STATUS_NO_MEM; goto cleanup;
                }

                if (!image.data) { // Check if buffer is valid
                     qWarning() << "Image buffer is null during read.";
                     status = SANE_STATUS_IO_ERROR;
                     goto cleanup;
                }

                unsigned char* current_pos = image.data + (image.y * image.width * components * bytes_per_pixel) + (image.x * components * bytes_per_pixel);
                 // This assumes interleaved data storage. Planar for R/G/B needs different logic.

                 if (parm.format == SANE_FRAME_GRAY || parm.format == SANE_FRAME_RGB) {
                     // Copy directly if gray or interleaved RGB
                     for (i = 0; i < len; ++i) {
                         // Ensure image.data is valid and large enough before writing
                         size_t required_offset = offset + 3 * i;
                         // Add proper size check based on image dimensions and depth
                         // if (required_offset >= allocated_buffer_size) { /* handle error/realloc */ }

                         image.data[required_offset] = buffer[i];

                         // Now advance returns bool, check for failure
                         if (!advance(&image)) // Call advance AFTER accessing the current pixel
                         {
                             qWarning() << "Advance failed, possibly out of memory.";
                             status = SANE_STATUS_NO_MEM;
                             goto cleanup;
                         }
                     }
                     offset += 3 * len; // Update offset after the loop
                     break;
                 } else { // Planar R, G, B - requires stride
                     int stride = 3 * bytes_per_pixel; // Assuming interleaved final buffer
                     for (i = 0; i < len; ++i) {
                         size_t pixel_index = (image.y * image.width + image.x);
                         // Check bounds
                         if (pixel_index * stride + offset >= required_size) {
                              qWarning() << "Buffer overflow detected in planar copy.";
                              status = SANE_STATUS_IO_ERROR; goto cleanup;
                         }
                         image.data[pixel_index * stride + offset] = buffer[i]; // Place R/G/B in correct plane/offset
                         image.x++;
                         if (image.x >= image.width) {
                             image.x = 0;
                             image.y++;
                             // Potentially check/realloc height here
                         }
                     }
                 }
                 // --- End Buffering Logic Update ---

            } else { // --- Write directly to PNG file ---
                int current_byte = 0;
                int bytes_remaining_in_buffer = len;

                while (bytes_remaining_in_buffer > 0) {
                    int bytes_to_copy = qMin(bytes_remaining_in_buffer, parm.bytes_per_line - pngrow);
                    memcpy(pngbuf + pngrow, buffer + current_byte, bytes_to_copy);

                    pngrow += bytes_to_copy;
                    current_byte += bytes_to_copy;
                    bytes_remaining_in_buffer -= bytes_to_copy;

                    // If a full row is ready, write it
                    if (pngrow == parm.bytes_per_line) {
                        // Handle 1-bit depth inversion if needed (as in original)
                         if (parm.depth == 1) {
                             for (int j = 0; j < parm.bytes_per_line; j++)
                                 pngbuf[j] = ~pngbuf[j];
                         }
                         // Check if png_ptr is valid before writing
                         if (png_ptr && info_ptr) {
                             png_write_row(png_ptr, pngbuf);
                         } else {
                             qWarning() << "png_ptr or info_ptr is null, cannot write row.";
                              status = SANE_STATUS_IO_ERROR; // Or another appropriate error
                              goto cleanup;
                         }
                        pngrow = 0; // Reset row buffer position
                    }
                }
                // --- End Direct PNG Write ---
            }
        } // End while(true) read loop

        first_frame = 0; // Mark subsequent loops as not the first frame
    } while (!parm.last_frame);


    // --- Finalize based on buffering ---
    if (must_buffer) {
        // --- Write buffered image to PNG ---
        qDebug() << "Writing buffered image data to PNG.";
        if (image.data) {
             // Update final image height based on how far 'y' advanced
             image.height = image.y + (image.x > 0 ? 1 : 0); // Include last partial line if any
             qDebug() << "Final buffered image height:" << image.height;

             // Write the PNG header now that dimensions are known
             write_png_header(parm.format, parm.pixels_per_line, image.height, parm.depth, ofp, &png_ptr, &info_ptr);
             if (!png_ptr || !info_ptr) {
                 qWarning() << "Failed to write PNG header for buffered image.";
                 status = SANE_STATUS_IO_ERROR;
                 goto cleanup;
             }

             // Write the image data row by row
             int components = (parm.format == SANE_FRAME_RGB) ? 3 : 1;
             int bytes_per_pixel = (parm.depth > 8) ? 2 : 1;
             size_t row_stride = (size_t)image.width * components * bytes_per_pixel; // Use calculated width

             for (int y = 0; y < image.height; ++y) {
                  png_bytep row_ptr = image.data + y * row_stride;
                  png_write_row(png_ptr, row_ptr);
             }

        } else {
            qWarning() << "Buffered image data is null, cannot write PNG.";
            status = SANE_STATUS_IO_ERROR; // Or appropriate error
            // No need for goto cleanup here, will fall through to png_write_end check
        }
        // --- End Buffered Write ---
    } else {
         // If not buffering, check if there's a partial row left in pngbuf
         if (pngrow > 0) {
             qWarning() << "Warning: Scan ended with partial row data (" << pngrow << " bytes). This may indicate an incomplete scan or incorrect parameters.";
             // Optionally, pad the rest of the row with zeros/white and write it?
             // memset(pngbuf + pngrow, 0, parm.bytes_per_line - pngrow);
             // png_write_row(png_ptr, pngbuf);
         }
    }


    // Finalize PNG writing if png_ptr was initialized
    if (png_ptr && info_ptr) {
        png_write_end(png_ptr, info_ptr);
    }


    /* flush the output buffer */
    fflush(ofp); // ofp should still be valid here


cleanup:
    // Store final status before cleanup might change it
    SANE_Status final_status = status;

    qDebug() << "Cleaning up scan resources. Final status:" << sane_strstatus(final_status);

    // Cleanup PNG resources
    if (png_ptr || info_ptr) { // Check before destroying
        png_destroy_write_struct(&png_ptr, &info_ptr); // Safe to call with null pointers too
    }
    free(pngbuf); // Safe to call free(NULL)

    // Cleanup image buffer if allocated
    if (image.data) {
        free(image.data);
        image.data = nullptr; // Avoid double free
    }

    // Cleanup sane_read buffer
    free(buffer);

    // Decide whether to cancel/close based on status
    // If EOF was reached, the scan completed naturally.
    // If another error occurred, maybe cancel? SANE docs are best here.
    // sane_cancel(m_device); // Might already be implicitly cancelled by error

    // Do NOT call sane_close or sane_exit here. Let ScannerDevice manage that.

    return final_status; // Return the status captured before cleanup
}
#endif // !_WIN32 