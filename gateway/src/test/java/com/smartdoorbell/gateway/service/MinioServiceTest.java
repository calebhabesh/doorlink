package com.smartdoorbell.gateway.service;

import com.amazonaws.services.s3.AmazonS3;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.web.multipart.MultipartFile;

import java.io.ByteArrayInputStream;
import java.io.InputStream;

import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.*;

@ExtendWith(MockitoExtension.class)
public class MinioServiceTest {

    @Mock
    private AmazonS3 amazonS3;

    @Mock
    private MultipartFile multipartFile;

    private MinioService minioService;

    @BeforeEach
    public void setup() {
        minioService = new MinioService(amazonS3);
        minioService.setBucketName("test-bucket");
    }

    @Test
    public void testUploadFile() throws Exception {
        when(multipartFile.getInputStream()).thenReturn(new ByteArrayInputStream("test data".getBytes()));
        when(multipartFile.getOriginalFilename()).thenReturn("test.jpg");
        when(multipartFile.getContentType()).thenReturn("image/jpeg");
        when(multipartFile.getSize()).thenReturn(9L);

        when(amazonS3.doesBucketExistV2("test-bucket")).thenReturn(true);

        String key = minioService.uploadFile(multipartFile);
        assertNotNull(key);
        verify(amazonS3, times(1)).putObject(eq("test-bucket"), any(String.class), any(InputStream.class), any());
    }
}
