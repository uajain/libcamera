/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Google Inc.
 *
 * core_ipa_interface.cpp - Docs file for core.mojom generated header
 */

namespace libcamera {

/**
 * \struct IPABuffer
 * \brief Buffer information for the IPA interface
 *
 * The IPABuffer structure associates buffer memory with a unique ID. It is
 * used to map buffers to the IPA with IPAInterface::mapBuffers(), after which
 * buffers will be identified by their ID in the IPA interface.
 */

/**
 * \fn IPABuffer::IPABuffer(uint32_t id, const std::vector<FrameBuffer::Plane> &planes)
 * \param[in] id
 * \param[in] planes
 * \sa id and planes
 */

/**
 * \var IPABuffer::id
 * \brief The buffer unique ID
 *
 * Buffers mapped to the IPA are identified by numerical unique IDs. The IDs
 * are chosen by the pipeline handler to fulfil the following constraints:
 *
 * - IDs shall be positive integers different than zero
 * - IDs shall be unique among all mapped buffers
 *
 * When buffers are unmapped with IPAInterface::unmapBuffers() their IDs are
 * freed and may be reused for new buffer mappings.
 */

/**
 * \var IPABuffer::planes
 * \brief The buffer planes description
 *
 * Stores the dmabuf handle and length for each plane of the buffer.
 */

/**
 * \struct IPASettings
 * \brief IPA interface initialization settings
 *
 * The IPASettings structure stores data passed to the IPAInterface::init()
 * function. The data contains settings that don't depend on a particular camera
 * or pipeline configuration and are valid for the whole life time of the IPA
 * interface.
 */

/**
 * \fn IPASettings::IPASettings(const std::string &configurationFile, const std::string &sensorModel)
 * \param[in] configurationFile
 * \param[in] sensorModel
 * \sa configurationFile and sensorModel
 */

/**
 * \var IPASettings::configurationFile
 * \brief The name of the IPA configuration file
 *
 * This field may be an empty string if the IPA doesn't require a configuration
 * file.
 */

/**
 * \var IPASettings::sensorModel
 * \brief The sensor model name
 *
 * Provides the sensor model name to the IPA.
 */

/**
 * \struct IPAStream
 * \brief Stream configuration for the IPA interface
 *
 * The IPAStream structure stores stream configuration parameters needed by the
 * IPAInterface::configure() method. It mirrors the StreamConfiguration class
 * that is not suitable for this purpose due to not being serializable.
 */

/**
 * \fn IPAStream::IPAStream(uint32_t pixelFormat, const Size &size)
 * \param[in] pixelFormat
 * \param[in] size
 * \sa pixelFormat and size
 */

/**
 * \var IPAStream::pixelFormat
 * \brief The stream pixel format
 */

/**
 * \var IPAStream::size
 * \brief The stream size in pixels
 */

} /* namespace libcamera */
