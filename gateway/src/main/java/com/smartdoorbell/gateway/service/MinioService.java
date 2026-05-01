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
}
