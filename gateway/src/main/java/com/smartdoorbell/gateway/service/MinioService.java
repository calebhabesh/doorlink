package com.smartdoorbell.gateway.service;

import com.amazonaws.services.s3.AmazonS3;
import com.amazonaws.services.s3.model.ObjectMetadata;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

import java.io.IOException;
import java.util.UUID;

@Service
public class MinioService {

    private final AmazonS3 amazonS3;

    @Value("${minio.bucket.name}")
    private String bucketName;

    public MinioService(AmazonS3 amazonS3) {
        this.amazonS3 = amazonS3;
    }

    public void setBucketName(String bucketName) {
        this.bucketName = bucketName;
    }

    public String uploadFile(MultipartFile file) throws IOException {
        if (!amazonS3.doesBucketExistV2(bucketName)) {
            amazonS3.createBucket(bucketName);
        }

        String originalFilename = file.getOriginalFilename();
        String extension = originalFilename != null && originalFilename.contains(".") ? originalFilename.substring(originalFilename.lastIndexOf(".")) : "";
        String key = UUID.randomUUID().toString() + extension;

        ObjectMetadata metadata = new ObjectMetadata();
        metadata.setContentType(file.getContentType());
        metadata.setContentLength(file.getSize());

        amazonS3.putObject(bucketName, key, file.getInputStream(), metadata);

        return key;
    }

    public String getPresignedUrl(String key) {
        if (key == null || key.isEmpty()) return null;
        java.util.Date expiration = new java.util.Date();
        long expTimeMillis = expiration.getTime();
        expTimeMillis += 1000 * 60 * 60; // 1 hour expiration
        expiration.setTime(expTimeMillis);
        
        com.amazonaws.services.s3.model.GeneratePresignedUrlRequest generatePresignedUrlRequest = 
                new com.amazonaws.services.s3.model.GeneratePresignedUrlRequest(bucketName, key)
                .withMethod(com.amazonaws.HttpMethod.GET)
                .withExpiration(expiration);
        return amazonS3.generatePresignedUrl(generatePresignedUrlRequest).toString();
    }

    public com.amazonaws.services.s3.model.S3Object getFile(String key) {
        if (key == null || key.isEmpty()) return null;
        try {
            return amazonS3.getObject(bucketName, key);
        } catch (Exception e) {
            return null;
        }
    }

    public void deleteFile(String key) {
        if (key == null || key.isEmpty()) return;
        try {
            amazonS3.deleteObject(bucketName, key);
        } catch (Exception e) {
            System.err.println("Failed to delete file from MinIO: " + e.getMessage());
        }
    }

    public boolean isAvailable() {
        try {
            return amazonS3.doesBucketExistV2(bucketName);
        } catch (Exception e) {
            return false;
        }
    }
}
